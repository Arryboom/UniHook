#include "MainWindow.h"
#include "ui_MainWindow.h"


#include "DlgFind.h"
#include "DlgProcessList.h"

#include <QtGui>
#include <QtCore>
#include <QDebug>
#include <QMessageBox>
#include <QThread>
#include <QTimer>
#include <QDebug>
#include <QStringList>

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <../Common/IPC/SharedMemQueue.h>
#include <../UniHookLoader/Injector.h>
#include "injector.h"

#pragma comment( lib, "psapi.lib" )
#pragma comment( lib, "user32.lib" )
#pragma comment( lib, "advapi32.lib" )

MainWindow::MainWindow( QWidget* pParent ) : QMainWindow( pParent ), ui( new Ui::MainWindow )
{
	ui->setupUi( this );

	m_pStatusLabel = new QLabel( this );
	ui->statusBar->addPermanentWidget( this->m_pStatusLabel, 1 );

	this->m_Pid = 0;
	this->m_hProcess = NULL;
	this->m_Injected = FALSE;

	ui->tblRoutines->setSelectionBehavior( QAbstractItemView::SelectRows );
	ui->tblRoutines->horizontalHeader( )->setStretchLastSection( true );
	ui->tblRoutines->horizontalHeader( )->setDefaultAlignment( Qt::AlignLeft );
	ui->tblRoutines->verticalHeader( )->setDefaultSectionSize( ui->tblRoutines->verticalHeader( )->fontMetrics( ).height( ) + 2 );

	ui->tblRoutines->setColumnCount( 4 );
	ui->tblRoutines->setHorizontalHeaderItem( 0, new QTableWidgetItem( "Icon" ) );
	ui->tblRoutines->setHorizontalHeaderItem( 1, new QTableWidgetItem( "Address" ) );
	ui->tblRoutines->setHorizontalHeaderItem( 2, new QTableWidgetItem( "Module" ) );
	ui->tblRoutines->setHorizontalHeaderItem( 3, new QTableWidgetItem( "Section" ) );

	UpdateStatusBar( );

	this->m_pIPCServer = new SharedMemQueue( "Local\\UniHook_IPC", 100000, SharedMemQueue::Mode::Server );

	MessageThread* pMessageThread = new MessageThread( this );
	pMessageThread->start( );
}

MainWindow::~MainWindow( )
{
    delete m_pIPCServer;
	delete ui;
}

void MainWindow::UpdateStatusBar( )
{
	if ( !this->m_hProcess )
	{
		this->m_pStatusLabel->setText( "No process selected" );
		return;
	}

	if ( this->m_Injected )
	{
		QString message = "Process: " + this->m_ProcessName + " (" + QString::number( this->m_Pid ) + "), module injected";
		this->m_pStatusLabel->setText( message );
	} else {
		QString message = "Process: " + this->m_ProcessName + " (" + QString::number( this->m_Pid ) + "), module injection failed";
		this->m_pStatusLabel->setText( message );
	}
}

void MainWindow::AddSubRoutine( QString address )
{
	int row = ui->tblRoutines->rowCount( );
	ui->tblRoutines->setRowCount( ui->tblRoutines->rowCount( ) + 1 );

	QIcon icon( ":/Images/Images/bullet_blue.png" );

	QTableWidgetItem* pIconItem = new QTableWidgetItem( );
	pIconItem->setIcon( icon );

	ui->tblRoutines->setItem( row, 0, pIconItem );
	ui->tblRoutines->setItem( row, 1, new QTableWidgetItem( address ) );

	/*
	bool converted;
	uintptr_t numeric = address.toULongLong( &converted, 16 );

	if ( converted )
	{
		MemoryRegion* pRegion = FindMemoryRegionByAddress( numeric );

		if ( pRegion )
		{
			ui->tblRoutines->setItem( row, 2, new QTableWidgetItem( pRegion->m_ModuleName ) );
			ui->tblRoutines->setItem( row, 3, new QTableWidgetItem( pRegion->m_SectionName ) );
		}
	} else {
		qDebug( ) << "[!] failed to convert address";
	}
	*/

	ui->tblRoutines->horizontalHeader( )->setSectionResizeMode( QHeaderView::ResizeToContents );
	ui->tblRoutines->verticalHeader( )->setSectionResizeMode( QHeaderView::ResizeToContents );
}

void MainWindow::on_action_Quit_triggered( )
{
	qApp->exit( );
}

void MainWindow::on_action_EnumSubRoutines_triggered( )
{
	qDebug( ) << "[+] on_action_EnumSubRoutines_triggered( )";
	this->m_pIPCServer->PushMessage( MemMessage( "ListSubs" ) );
}

void MainWindow::on_action_SelectProcess_triggered( )
{
	DlgProcessList dialog;
	dialog.setModal( true );

	if ( dialog.exec( ) == QDialog::Accepted )
	{
		HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, dialog.m_ProcessId );

		if ( !hProcess )
		{
			QMessageBox::critical( this, "Error", "OpenProcess( ) failed. No permission to open: " + dialog.m_ProcessName + "." );
			return;
		}

        this->m_pIPCServer->PushMessage(MemMessage("IPC Connection Initialized!"));
        if (!InjectDLL( hProcess,"C:\\Visual Studio 2015\\Git Source Control\\UniHook\\x64\\Release\\UniHook.dll"))
		{
			QMessageBox::critical( this, "Error", "InjectDLL( ) failed. Could not inject into: " + dialog.m_ProcessName + "." );
			return;
		}

		this->m_Pid			= dialog.m_ProcessId;
		this->m_ProcessName	= dialog.m_ProcessName;
		this->m_hProcess	= hProcess;
		this->m_Injected	= TRUE;

		UpdateMemoryList( );
		UpdateImageSections( );
		UpdateStatusBar( );

        this->m_pIPCServer->WaitForMessage();
        MemMessage Msg;
        if(m_pIPCServer->PopMessage(Msg))
            qDebug( ) << "[+] IPC Connected Succesfully";
        else
            qDebug( ) << "[+] IPC Connection Failed";
	}
}


void MainWindow::on_action_Find_triggered( )
{
	DlgFind dialog;
	dialog.setModal( true );

	if ( dialog.exec( ) != QDialog::Accepted )
		return;

	for ( int i = ui->tblRoutines->rowCount( ); i > 0; i-- )
	{
		QTableWidgetItem* pItem = ui->tblRoutines->item( i, 1 );

		if ( !pItem )
			continue;

		if ( pItem->text( ).indexOf( dialog.m_Address ) != -1 )
		{
			ui->tblRoutines->scrollToItem( pItem );
			ui->tblRoutines->selectRow( i );
		}
	}
}

void MainWindow::on_tblRoutines_customContextMenuRequested( const QPoint& pos )
{
	QMenu* menu = new QMenu( this );

	QAction* actCopyAddress = new QAction( QIcon( ":/Images/Images/clipboard.png" ), "Copy Address", this );
	actCopyAddress->setIconVisibleInMenu( true );
	menu->addAction( actCopyAddress );
	connect( actCopyAddress, SIGNAL( triggered( ) ), this, SLOT( CopySelectedAddress( ) ) );

	QAction* actHookRoutine = new QAction( QIcon( ":/Images/Images/bullet_go.png" ), "Hook Routine", this );
	actHookRoutine->setIconVisibleInMenu( true );
	menu->addAction( actHookRoutine );
	connect( actHookRoutine, SIGNAL( triggered( ) ), this, SLOT( HookSelectedRoutine( ) ) );

	menu->popup( ui->tblRoutines->viewport( )->mapToGlobal( pos ) );
}

void MainWindow::CopySelectedAddress( )
{
	QTableWidgetItem* pSelectedItem = ui->tblRoutines->selectedItems( ).at( 0 );

	if ( !pSelectedItem )
		return;

	QClipboard* pClipboard = QApplication::clipboard( );

	if ( !pClipboard )
		return;

	pClipboard->setText( ui->tblRoutines->item( pSelectedItem->row( ), 1 )->text( ) );
}

void MainWindow::HookSelectedRoutine( )
{
	qDebug( ) << "[+] MainWindow::HookSelectedRoutine( )";

	QTableWidgetItem* pSelectedItem = ui->tblRoutines->selectedItems( ).at( 0 );

	if ( !pSelectedItem )
		return;

	QClipboard* pClipboard = QApplication::clipboard( );

	if ( !pClipboard )
		return;

	QString message = "HookAtAddr|" + ui->tblRoutines->item( pSelectedItem->row( ), 1 )->text( );
	this->m_pIPCServer->PushMessage( MemMessage( message.toStdString( ) ) );
}

void MainWindow::UpdateMemoryList( )
{
	if ( !this->m_hProcess )
		return;

	MEMORY_BASIC_INFORMATION mbi = {0};
	uintptr_t address = 0;

	SYSTEM_INFO si;
	GetSystemInfo( &si );

	while ( address < reinterpret_cast<uintptr_t>( si.lpMaximumApplicationAddress ) )
	{
		SIZE_T size = VirtualQueryEx( this->m_hProcess,
									  reinterpret_cast<LPVOID>( address ), &mbi, sizeof( MEMORY_BASIC_INFORMATION ) );

		if ( size )
		{
			MemoryRegion* pRegion	= new MemoryRegion( );

			pRegion->m_BaseAddress	= reinterpret_cast<uintptr_t>( mbi.BaseAddress );
			pRegion->m_RegionSize	= mbi.RegionSize;
			pRegion->m_Type			= mbi.Type;
			pRegion->m_Protect		= mbi.Protect;
			pRegion->m_State		= mbi.State;

			wchar_t buffer[256] = {0};
			GetMappedFileName( this->m_hProcess, reinterpret_cast<LPVOID>( pRegion->m_BaseAddress ), buffer, sizeof( buffer ) );

			QFileInfo file( QString::fromWCharArray( buffer ) );
			pRegion->m_ModuleName = file.fileName( );

			this->m_Regions.push_back( pRegion );
			address += mbi.RegionSize;
		} else {
			address += si.dwPageSize;
		}
	}
}

void MainWindow::UpdateImageSections( )
{
	QVector<QString> completedModules;

	for ( int i = 0; i < this->m_Regions.size( ); i++ )
	{
		// Loop through executable sections

		MemoryRegion* pRegion = this->m_Regions.at( i );

		if ( !pRegion )
			continue;

		if ( !pRegion->m_ModuleName.isEmpty( ) && pRegion->m_Type == MEM_IMAGE )
		{
			foreach ( QString module, completedModules )
			{
				if ( module == pRegion->m_ModuleName )
					continue;
			}

			// Parse PE header

			IMAGE_DOS_HEADER dosheader;
			ZeroMemory( &dosheader, sizeof( IMAGE_DOS_HEADER ) );
			ReadProcessMemory( this->m_hProcess, reinterpret_cast<LPCVOID>( pRegion->m_BaseAddress ), &dosheader, sizeof( IMAGE_DOS_HEADER ), NULL );

			if ( dosheader.e_magic != IMAGE_DOS_SIGNATURE || dosheader.e_lfanew < 0 )
				continue;

			DWORD dwSignature;
			ReadProcessMemory( this->m_hProcess, reinterpret_cast<LPCVOID>( pRegion->m_BaseAddress + dosheader.e_lfanew ), &dwSignature, sizeof( DWORD ), NULL );

			if ( dwSignature != IMAGE_NT_SIGNATURE )
				continue;

			//

			IMAGE_NT_HEADERS ntheader;
			ZeroMemory( &ntheader, sizeof( IMAGE_NT_HEADERS ) );

			ReadProcessMemory( this->m_hProcess,
							   reinterpret_cast<LPCVOID>( pRegion->m_BaseAddress + dosheader.e_lfanew ),
							   &ntheader,
							   sizeof( IMAGE_NT_HEADERS ),
							   NULL );

			if ( ntheader.FileHeader.NumberOfSections == 0 )
				continue;

			// This section is usually the header
			// pRegion->m_SectionName = "IMAGE_DOS_HEADER";

			// Read individual section

			DWORD dwSize = ntheader.FileHeader.NumberOfSections * sizeof( IMAGE_SECTION_HEADER );

			PIMAGE_SECTION_HEADER pSectionHeader = reinterpret_cast<PIMAGE_SECTION_HEADER>( new char[dwSize] );
			ZeroMemory( pSectionHeader, dwSize );


			ReadProcessMemory( this->m_hProcess,
							   reinterpret_cast<LPCVOID>( pRegion->m_BaseAddress + dosheader.e_lfanew + sizeof( IMAGE_NT_HEADERS ) ),
							   pSectionHeader,
							   dwSize,
							   NULL );

			for ( int n = 0; n < ntheader.FileHeader.NumberOfSections; n++ )
			{
				uintptr_t address = pRegion->m_BaseAddress + pSectionHeader[n].VirtualAddress;

				MemoryRegion* pRegionResult = FindMemoryRegionByAddress( address );

				if ( pRegionResult )
				{
					QString sectionName;
					sectionName.sprintf( "%.8s", pSectionHeader[n].Name );
					pRegionResult->m_SectionName = sectionName;
				}

				pRegionResult->m_ModuleHandle = reinterpret_cast<HANDLE>( pRegion->m_BaseAddress );
			}

			// Done

			completedModules.push_back( pRegion->m_ModuleName );
		}
	}
}

MemoryRegion* MainWindow::FindMemoryRegionByAddress( uintptr_t address )
{
	for ( int i = 0; i < this->m_Regions.size( ); i++ )
	{
		MemoryRegion* pRegion = this->m_Regions.at( i );

		if ( !pRegion )
			continue;

		if ( address == pRegion->m_BaseAddress )
			return pRegion;

		// Regions are sorted by their address
		// if address greater than current region, then skip and increment to next region

		if ( address > pRegion->m_BaseAddress )
			continue;

		// This is the last entry in the list, return it

		if ( i + 1 > this->m_Regions.size( ) )
			return pRegion;

		// If greater than current address, make sure we are less than the next one

		MemoryRegion* pNextRegion = this->m_Regions.at( i + 1 );

		if ( !pNextRegion )
			return pRegion;

		if ( address < pNextRegion->m_BaseAddress )
			return pRegion;
	}

	return NULL;
}

MessageThread::MessageThread( MainWindow* pMainWindow )
{
	this->m_pMainWindow = pMainWindow;
}

void MessageThread::run( )
{
	qDebug( ) << "[+] MessageThread::run( )";

	this->m_pMainWindow->m_pIPCServer->WaitForMessage( );
	MemMessage message;

	while ( this->m_pMainWindow->m_pIPCServer->PopMessage( message ) )
	{
		QString data = QString::fromUtf8( reinterpret_cast<const char*>( &message.m_Data[0] ) );
		QStringList list = data.split( "|" );

		if ( list.at( 0 ) == "0" )
		{
			this->m_pMainWindow->AddSubRoutine( list.at( 2 ) );
		}
	}
}
