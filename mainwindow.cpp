#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	setWindowTitle("Mission Converter " + pmcVersion + "by PMC.");
}

MainWindow::~MainWindow()
{
	delete ui;
}


void MainWindow::on_pushButton_BrowseDir_clicked()
{
	sqmDir = QFileDialog::getExistingDirectory();

	ui->lineEdit->setText(sqmDir);
}


void MainWindow::on_pushButton_ScanOnly_clicked()
{
	QDirIterator it(sqmDir, QDirIterator::Subdirectories);

	bool pmcSqmFound = false;

	while (it.hasNext())
	{
		if (it.fileName().contains("mission.sqm"))
		{
			missionDirName = it.fileInfo().path();
			ui->textEdit->append(missionDirName);
			checkPMCDir(missionDirName);
			pmcSqmFound = true; taskTypeArmA2 = false; taskTypeOFP = false; taskTypeShuko = false;
			scanSqm(it.filePath(), missionDirName);
			scanOverviewHtml(it.fileInfo().absolutePath());
			scanDescriptionExt(it.fileInfo().absolutePath());
			scanInitsqf(it.fileInfo().absolutePath());
			scanBriefing(missionDirName);
		}

		it.next();
		taskID = 0;
	}

	if (!pmcSqmFound)
		ui->textEdit->append("No mission.sqm files found, sorry.");
	else
		ui->textEdit->append("All done, thank you.");
}


void MainWindow::scanSqm(QString sqm, QString missionDirName)
{
	QFile File(sqm);
	File.open(QIODevice::ReadOnly);
	QTextStream in(&File);

	sqm.truncate(sqm.length()-4);
	sqm.append("_Converted.sqm");
	QFile FileSqmNew(sqm);
	FileSqmNew.open(QIODevice::WriteOnly);
	QTextStream out(&FileSqmNew);

	while (!in.atEnd())
	{
		QString line = in.readLine();
		QString checkMeline = line.toLower();

		if (checkMeline.contains("shk_taskmaster_upd"))
		{
			taskTypeShuko = true;
			line = convertSqmLine(line);
			createTaskSqf(missionDirName, taskFilename, outputSqf);
		}

		if (checkMeline.contains("objstatus"))
		{
			taskTypeOFP = true;
			line = convertSqmLine(line);
			createTaskSqf(missionDirName, taskFilename, outputSqf);
		}

		if (checkMeline.contains("settaskstate"))
		{
			taskTypeArmA2 = true;
			line = convertSqmLine(line);
			createTaskSqf(missionDirName, taskFilename, outputSqf);
		}
		out << line << "\n";
	}

	File.close();
	FileSqmNew.close();
}


void MainWindow::scanDescriptionExt(QString dext)
{
	dext.append("/description.ext");

	bool fileExists = QFileInfo::exists(dext) && QFileInfo(dext).isFile();
	if (fileExists)
	{
		QFile File(dext);
		if (!File.open(QIODevice::ReadWrite))
		ui->textEdit->append("Opened description.ext successfully: " + File.fileName());
		QTextStream in(&File);
		QString line = in.readAll();
		overviewAddition.clear();
		checkDescriptionExtLine(line);
		if (overviewAddition.length() > 0)
		{
			in.seek(line.size());
			in << overviewAddition;
		}

		File.close();
	}
	else
	{
		ui->textEdit->append("No description.ext file found, creating new...");
		createDescriptionExt(dext);
	}
}


void MainWindow::scanOverviewHtml(QString overviewh)
{
	overviewh.append("/overview.html");
	QFile File(overviewh);

	if (!File.open(QIODevice::ReadOnly))
	{
		ui->textEdit->append("No overview.html file found.");
		return;
	}
	else
	{
		QTextStream in(&File);
		QString line = in.readAll();
		File.close();
		// delete overview.html, YIKES !
		File.remove();
		overviewBlob = extractOverview(line);
	}
}


void MainWindow::scanInitsqf(QString initsqf)
{
	initsqf.append("/init.sqf");
	QFile File(initsqf);

	if (!File.open(QIODevice::ReadOnly))
	{
		ui->textEdit->append("No init.sqf file found.");
		return;
	}
	else
	{
		QTextStream in(&File);
		QString line = in.readAll();
		checkInitSqf(line);

		File.close();
	}
}


void MainWindow::scanBriefing(QString missionDirName)
{
	if (taskTypeShuko)
	{
		QString myBriefing = missionDirName;
		myBriefing.append("/init.sqf");
		QFile File(myBriefing);

		if (!File.open(QIODevice::ReadOnly))
		{
			ui->textEdit->append("No " + myBriefing + " file found while searching for Shuko.");
			return;
		}
		else
		{
			File.close();
		}
	}

	if (taskTypeArmA2)
	{
		QString myBriefing = missionDirName;
		myBriefing.append("/briefing.sqf");
		QFile File(myBriefing);

		if (!File.open(QIODevice::ReadOnly))
		{
			ui->textEdit->append("No " + myBriefing + " file found while searching for ArmA 2.");
			return;
		}
		else
		{
			// source briefing.sqf
			QTextStream in(&File);

			QFile BriefingNew(missionDirName + "/PMC/PMC_Briefing.sqf");
			if (!BriefingNew.open(QIODevice::WriteOnly))
			{
				ui->textEdit->append("Cannot open " + missionDirName + "/PMC/PMC_Briefing.sqf");
				return;
			}
			QTextStream outBriefing(&BriefingNew);

			QFile TasksSqf(missionDirName + "/PMC/PMC_Tasks.sqf");
			if (!TasksSqf.open(QIODevice::WriteOnly))
			{
				ui->textEdit->append("Cannot open " + missionDirName + "/PMC/PMC_Tasks.sqf");
				return;
			}
			QTextStream outTasks(&TasksSqf);

			while (!in.atEnd())
			{
				QString line = in.readLine();

				// briefing stuff
				// waitUntil {!(isNull player)};
				if (!line.contains("createSimpleTask") && !line.contains("ArmaBriefingConversion") && !line.contains("setSimpleTaskDescription") && !line.contains("// tasks need to be in reversed order") && line.length() > 0)
				{
					line.replace(";", ";\n");
					outBriefing << line;
				}

				// task stuff
				if (line.contains("createSimpleTask"))
				{
					line = parseTaskCreate(line);
					outTasks << line << "\n";
				}
			}
			File.close();
			BriefingNew.close();
			TasksSqf.close();
		}
	}
}


void MainWindow::checkDescriptionExtLine(QString line)
{
	line = line.toLower();
	if (!line.contains("overviewtext")) overviewAddition.append(overviewBlob);
	if (!line.contains("overviewpicture")) overviewAddition.append("\noverviewPicture = \"\";");
	if (!line.contains("onloadname")) overviewAddition.append("\nonloadname = \"\";");
	if (!line.contains("onloadmission")) overviewAddition.append("\nonloadmission = \"\";");
	if (!line.contains("onloadintro")) overviewAddition.append("\nonloadintro = \"\";");
	if (!line.contains("author")) overviewAddition.append("\nauthor = \"\";");
	if (!line.contains("cfgdebriefing"))
	{
		overviewAddition.append("\n\nclass CfgDebriefing\n{\n\tclass pmc_end1\n\t{\n\t\ttitle = \"Mission Accomplished!\";\n\t\tdescription = \"Congratulations\";\n\t};\n\n");
		overviewAddition.append("\tclass pmc_end2\n\t{\n\t\ttitle = \"Mission Lost!\";\n\t\tdescription = \"You failed!\";\n\t};\n};\n");
	}
}


void MainWindow::checkInitSqf(QString line)
{
	line = line.toLower();
	if (line.contains("shk_taskmaster.sqf"))
	{
		ui->textEdit->append("init.sqf -> SHK_Taskmaster.sqf detected.");
		taskTypeShuko = true;
	}
	if (line.contains("briefing.sqf"))
	{
		ui->textEdit->append("init.sqf -> briefing.sqf detected.");
		taskTypeArmA2 = true;
	}
}


QString MainWindow::convertSqmLine(QString line)
{
	QString originalLine = line;
	outputSqf.clear();
	taskFilename.clear();

	// remove onActivateion=" and expActiv="
	line.replace("onActivation=\"", "");
	line.replace("expActiv=\"", "");
	// replace "" with "
	line.replace("\"\"", "\"");
	line.truncate(line.length()-2);
	taskID++;
	outputSqf = sqfHeader() + "/*\n\n\ttask ID should be: " + QString::number(taskID) + "\n\n*/\n";
	line = line.simplified();
	outputSqf.append(line);
	outputSqf.replace(";", ";\n");

	// this is so lame and SO needs an regular expression ...
	outputSqf.replace("\"1\" objStatus \"DONE\"", "[\"t1\", \"SUCCEEDED\", true] spawn BIS_fnc_taskSetState");
	outputSqf.replace("\"2\" objStatus \"DONE\"", "[\"t2\", \"SUCCEEDED\", true] spawn BIS_fnc_taskSetState");
	outputSqf.replace("\"3\" objStatus \"DONE\"", "[\"t3\", \"SUCCEEDED\", true] spawn BIS_fnc_taskSetState");
	outputSqf.replace("\"4\" objStatus \"DONE\"", "[\"t4\", \"SUCCEEDED\", true] spawn BIS_fnc_taskSetState");
	outputSqf.replace("\"5\" objStatus \"DONE\"", "[\"t5\", \"SUCCEEDED\", true] spawn BIS_fnc_taskSetState");
	outputSqf.replace("\"6\" objStatus \"DONE\"", "[\"t6\", \"SUCCEEDED\", true] spawn BIS_fnc_taskSetState");
	outputSqf.replace("\"7\" objStatus \"DONE\"", "[\"t7\", \"SUCCEEDED\", true] spawn BIS_fnc_taskSetState");
	outputSqf.replace("\"8\" objStatus \"DONE\"", "[\"t8\", \"SUCCEEDED\", true] spawn BIS_fnc_taskSetState");
	outputSqf.replace("\"9\" objStatus \"DONE\"", "[\"t9\", \"SUCCEEDED\", true] spawn BIS_fnc_taskSetState");
	outputSqf.replace("\"10\" objStatus \"DONE\"", "[\"t10\", \"SUCCEEDED\", true] spawn BIS_fnc_taskSetState");

	// this is even dirtier, eew
	outputSqf.replace("setTaskState \"SUCCEEDED\"", ", \"SUCCEEDED\", true] spawn BIS_fnc_taskSetState");
	outputSqf.replace("setTaskState \"FAILED\"", ", \"FAILED\", true] spawn BIS_fnc_taskSetState");

	taskFilename = "PMC_Objective" + QString::number(taskID) + ".sqf";

	if (originalLine.contains("onActivation")) originalLine = "onActivation=\"0 = [] execVM \"\"PMC\\" + taskFilename + "\"\";\";";
	if (originalLine.contains("expActiv")) originalLine = "expActiv=\"0 = [] execVM \"\"PMC\\" + taskFilename + "\"\";\";";

	return originalLine;
}


void MainWindow::checkPMCDir(QString line)
{
	QDir file;
	file.cd(line);
	if (!file.exists("PMC"))
	{
		if (!file.mkdir("PMC")) ui->textEdit->append("PMC scripts sub dir creation failed :-(");
	}
}


void MainWindow::createTaskSqf(QString missionDirName, QString fileName, QString taskLine)
{
	QDir theDir;
	theDir.cd(missionDirName);

	QFile File(theDir.absolutePath() + "/PMC/" + fileName);
	if (!File.open(QIODevice::WriteOnly))
		ui->textEdit->append("Cannot open filename: " + fileName);
	else
	{
		ui->textEdit->append("Created " + File.fileName());
		QTextStream out(&File);
		out << taskLine << "\n";
		File.close();
	}
}


QString MainWindow::extractOverview(QString line)
{
	line.replace("<html>", "");
	line.replace("</html>", "");
	line.replace("<head>", "");
	line.replace("</head>", "");
	line.replace("<title>Overview</title>", "");
	line.replace("<body>", "");
	line.replace("</body>", "");
	line.replace("<p align=\"center\">", "");
	line.replace("<h2 align=\"center\">", "");
	line.replace("<br>", "");
	line.replace("<BR>", "");
	line.replace("<p>", "");
	line.replace("</p>", "");
	line.replace("<img src=\"overview_mission_ca.paa\"", "overviewPicture = \"overview_mission_ca.paa\";");
	line.replace("<h2>", "");
	line.replace("</h2>", "");
	line.replace("<a name=\"Main\">", "");
	line.replace("</a>", "");
	line.replace("<! --- ----------------------------->", "");
	line.replace("<! ---  TEXT FOR MISSION TITLE   --->", "");
	line.replace("<! ---    TEXT FOR DESCRIPTION	 --->", "");
	line.replace("<! --- END OF OVERVIEW>", "");
	line = line.simplified();
	line.prepend(sqfHeader() + "\noverviewText = \"");
	line.append("\";");

	return line;
}


void MainWindow::createDescriptionExt(QString dext)
{
	QFile File(dext);
	if (!File.open(QIODevice::WriteOnly))
		ui->textEdit->append("Cannot create description.ext " + dext);
	else
	{
		ui->textEdit->append("Created " + File.fileName());
		QTextStream out(&File);
		out << overviewBlob << "\n";

		// create resource file for this junk, then use replace string and take values from GUI or something
		out << "author = \"Snake Man, PMC.\";\n";
		out << "onLoadName = \"\";\n";
		out << "onLoadMission = \"https://www.pmctactical.org\";\n";
		out << "onLoadMissionTime = 1;\n";
		out << "onLoadIntro = \"https://www.pmctactical.org\";\n";
		out << "onLoadIntroTime = 1;\n";
		out << "overviewPicture = \"\";\n";
		out << "\nclass CfgDebriefing\n{\n\tclass pmc_end1\n\t{\n\t\ttitle = \"Mission Accomplished!\";\n\t\tdescription = \"Congratulations\";\n\t};\n";
		out << "\n\tclass pmc_end2\n\t{\n\t\ttitle = \"Mission Lost!\";\n\t\tdescription = \"You failed!\";\n\t};\n};\n";

		File.close();
	}
}


QString MainWindow::parseTaskCreate(QString line)
{
	QStringList list;
	list = line.split(" ");
	QString temps = "[player,[\"" + list[0].simplified() + "\"],[\"";
	list = line.split("\"");
	temps.append(list[1].simplified() + "\",\"" + list[1].simplified() + "\",\"" + list[1].simplified() + "\"],objNull,1,2,true] call BIS_fnc_taskCreate;");
	// doesnt work here, you have to detect somehow the LAST task, not after every task hehe
	//temps.append("\n\n[\"t1\", \"ASSIGNED\"] call BIS_fnc_taskSetState;");

	return temps;
}


QString MainWindow::sqfHeader()
{
	QDateTime date = QDateTime::currentDateTimeUtc();
	QString utctime = date.toString(Qt::ISODate);

	QString sqfhdr = "/*\n\n\tCreated at " + utctime + " with Mission Converter " + pmcVersion + " by PMC\n\n*/\n";

	return sqfhdr;
}
