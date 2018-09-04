#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	setWindowTitle("Mission Converter " + pmcVersion + "by PMC.");
	// debug hardcoded for my pleasure
	sqmDir = "D:/coding/Test_files/Arma3_Missions";
	on_pushButton_ScanOnly_clicked();
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
			//ui->textEdit->append("call SHK_Taskmaster_Upd detected " + line);
			//ui->textEdit->append(convertSqmLine(line));
			// we received new SQM onActivation / expActiv line which we can now save
			line = convertSqmLine(line);
			createTaskSqf(missionDirName, taskFilename, outputSqf);
		}

		if (checkMeline.contains("objstatus"))
		{
			taskTypeOFP = true;
			//ui->textEdit->append("objStatus detected " + line);
			//ui->textEdit->append(convertSqmLine(line));
			// we received new SQM onActivation / expActiv line which we can now save
			line = convertSqmLine(line);
			createTaskSqf(missionDirName, taskFilename, outputSqf);
		}

		if (checkMeline.contains("settaskstate"))
		{
			taskTypeArmA2 = true;
			//ui->textEdit->append("setTaskState detected " + line);
			//ui->textEdit->append(convertSqmLine(line));
			// we received new SQM onActivation / expActiv line which we can now save
			line = convertSqmLine(line);
			createTaskSqf(missionDirName, taskFilename, outputSqf);
		}
		// write new sqm file line
		out << line << "\n";
	}

	File.close();
	FileSqmNew.close();
}


void MainWindow::scanDescriptionExt(QString dext)
{
	dext.append("/description.ext");
	QFile File(dext);

	if (!File.open(QIODevice::ReadWrite | QIODevice::Append))
	{
		ui->textEdit->append("No description.ext file found, creating new...");
		createDescriptionExt(dext);
	}
	else
	{
		QTextStream in(&File);
		QString line = in.readAll();
		checkDescriptionExtLine(line);

		File.close();
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
	QString myBriefing = missionDirName;
	myBriefing.append("/briefing.sqf");
	QFile File(myBriefing);

	if (!File.open(QIODevice::ReadOnly))
	{
		ui->textEdit->append("No " + myBriefing + " file found.");
		return;
	}
	else
	{
		// source briefing.sqf
		QTextStream in(&File);

		// PMC/PMC_Briefing.sqf
		QFile BriefingNew(missionDirName + "/PMC/PMC_Briefing.sqf");
		if (!BriefingNew.open(QIODevice::WriteOnly))
		{
			ui->textEdit->append("Cannot open " + missionDirName + "/PMC/PMC_Briefing.sqf");
			return;
		}
		QTextStream outBriefing(&BriefingNew);

		// PMC/PMC_Tasks.sqf
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
			//ui->textEdit->append(line);
			// waitUntil {!(isNull player)};
			//if (line.contains("createDiaryRecord")) outBriefing << line << "\n";
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


void MainWindow::checkDescriptionExtLine(QString line)
{
	line = line.toLower();
	if (!line.contains("overviewtext")) ui->textEdit->append("Missing description.ext/overviewText");
	if (!line.contains("overviewpicture")) ui->textEdit->append("Missing description.ext/overviewPicture");
	if (!line.contains("onloadname")) ui->textEdit->append("Missing description.ext/onLoadName");
	if (!line.contains("onloadmission")) ui->textEdit->append("Missing description.ext/onLoadMission");
	if (!line.contains("onloadintro")) ui->textEdit->append("Missing description.ext/onLoadIntro");
	if (!line.contains("author")) ui->textEdit->append("Missing description.ext/author");
}


void MainWindow::checkInitSqf(QString line)
{
	line = line.toLower();
	if (line.contains("shk_taskmaster.sqf")) ui->textEdit->append("init.sqf -> SHK_Taskmaster.sqf detected.");
	if (line.contains("briefing.sqf")) ui->textEdit->append("init.sqf -> briefing.sqf detected.");
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
	outputSqf = "/*\n\n\tcreated with Mission Converter by PMC\n\n\ttask ID should be: " + QString::number(taskID) + "\n\n*/\n\n";
	line = line.simplified();
	outputSqf.append(line);
	outputSqf.replace(";", ";\n");
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
	//line.replace("<>", "");
	//line.replace("</>", "");
	line = line.simplified();
	line.prepend("/*\n\n\tcreated with Mission Converter by PMC\n\n*/\n\noverviewText = \"");
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
		File.close();
	}
}


QString MainWindow::parseTaskCreate(QString line)
{
	QStringList list;
	list = line.split(" ");
	// 0 - objective variable name
	// 1 - =
	// 2 - player
	// 3 - createSimpleTask
	// 4 - beginning of the array
	QString temps = "[player,[\"" + list[0] + "\"],[\"";
	list = line.split("\"");
	//ui->textEdit->append("list[]: " + list[1]);
	temps.append(list[1] + "\",\"" + list[1] + "\",\"" + list[1] + "\"],objNull,1,2,true] call BIS_fnc_taskCreate;");
	// doesnt work here, you have to detect somehow the LAST task, not after every task hehe
	//temps.append("\n\n[\"t1\", \"ASSIGNED\"] call BIS_fnc_taskSetState;");
	ui->textEdit->append(temps);
	//ui->textEdit->append(line);
	// tasks need to be in reversed order
	//objective4 = player createSimpleTask ["Target 4"];
	return temps;
}
