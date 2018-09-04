#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	void on_pushButton_BrowseDir_clicked();

	void on_pushButton_ScanOnly_clicked();

private:
	Ui::MainWindow *ui;
	QString sqmDir;
	void scanSqm(QString sqm, QString missionDirName);
	void scanDescriptionExt(QString dext);
	void checkDescriptionExtLine(QString line);
	QString pmcVersion = "v0.0.1";
	void scanOverviewHtml(QString dext);
	void scanInitsqf(QString initsqf);
	void checkInitSqf(QString line);
	QString convertSqmLine(QString line);
	int taskID = 0;
	QString outputSqf, taskFilename, missionDirName, overviewBlob;
	void checkPMCDir(QString pmcPath);
	void createTaskSqf(QString missionDirName, QString fileName, QString taskLine);
	QString extractOverview(QString line);
	void createDescriptionExt(QString missionDirName);
	bool taskTypeShuko = false;
	bool taskTypeOFP = false;
	bool taskTypeArmA2 = false;
	void scanBriefing(QString missionDirName);
	QString parseTaskCreate(QString line);
};

#endif // MAINWINDOW_H
