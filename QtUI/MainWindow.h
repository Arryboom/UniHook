#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QLabel>

#include "Windows.h"
#include "Regions.h"

namespace Ui
{
	class MainWindow;
}

class SharedMemQueue;
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow( QWidget* pParent = 0 );
	~MainWindow( );

	void UpdateStatusBar( );
	void AddSubRoutine( QString address );
	void UpdateMemoryList( );
	void UpdateImageSections( );
	MemoryRegion* FindMemoryRegionByAddress( uintptr_t address );

	SharedMemQueue*			m_pIPCServer;
	QVector<MemoryRegion*>	m_Regions;
	HANDLE					m_hProcess;

private slots:
	void on_action_Quit_triggered( );
	void on_action_EnumSubRoutines_triggered( );
	void on_action_SelectProcess_triggered( );
	void on_action_Find_triggered( );
	void on_tblRoutines_customContextMenuRequested( const QPoint& pos );
	void CopySelectedAddress( );
	void HookSelectedRoutine( );

private:
	Ui::MainWindow* ui;

	QLabel*		m_pStatusLabel;
	QString		m_ProcessName;
	DWORD		m_Pid;	
	BOOL		m_Injected;
};


#endif // MAINWINDOW_H
