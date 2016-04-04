#include "DlgProcessList.h"
#include "ui_DlgProcessList.h"

#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPixmap>
#include <tlhelp32.h>
#include <psapi.h>

//QT_BEGIN_NAMESPACE
//Q_GUI_EXPORT HBITMAP qt_pixmapToWinHBITMAP( const QPixmap &p, int hbitmapFormat = 0 );
//Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON( HICON icon );

DlgProcessList::DlgProcessList( QWidget* pParent ) : QDialog( pParent ), ui( new Ui::DlgProcessList )
{
	ui->setupUi( this );

	ui->tblProcessList->setSelectionBehavior( QAbstractItemView::SelectRows );

	ui->tblProcessList->horizontalHeader( )->setStretchLastSection( true );
	ui->tblProcessList->horizontalHeader( )->setDefaultAlignment( Qt::AlignLeft );

	ui->tblProcessList->verticalHeader( )->setDefaultSectionSize( ui->tblProcessList->verticalHeader( )->fontMetrics( ).height( ) + 4 );

	ui->tblProcessList->setHorizontalHeaderItem( 0, new QTableWidgetItem( "Name" ) );
	ui->tblProcessList->setHorizontalHeaderItem( 1, new QTableWidgetItem( "PID" ) );

	RefreshProcessList( );
}

void DlgProcessList::RefreshProcessList( )
{
	ui->tblProcessList->setSortingEnabled( false );

	HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

	if ( hSnapshot == INVALID_HANDLE_VALUE )
	{
		qDebug( ) << "[!] Failed to call CreateToolhelp32Snapshot, GetLastError( )" <<  GetLastError( );
		return;
	}

	PROCESSENTRY32 pe = { 0 };
	pe.dwSize = sizeof( PROCESSENTRY32 );

	for ( BOOL	success =  Process32First( hSnapshot, &pe );
				success == TRUE;
				success =  Process32Next ( hSnapshot, &pe ) )
	{

		QString name = QString::fromWCharArray( pe.szExeFile );

		int row = ui->tblProcessList->rowCount( );

		ui->tblProcessList->setRowCount( ui->tblProcessList->rowCount( ) + 1 );
		ui->tblProcessList->setItem( row, 0, new QTableWidgetItem( name ) );
		ui->tblProcessList->setItem( row, 1, new QTableWidgetItem( QString::number( pe.th32ProcessID ) ) );
	}

	CloseHandle( hSnapshot );

	ui->tblProcessList->horizontalHeader( )->setSectionResizeMode( QHeaderView::ResizeToContents );
	ui->tblProcessList->setSortingEnabled( true );
}

DlgProcessList::~DlgProcessList( )
{
	delete ui;
}

void DlgProcessList::on_txtFilter_textChanged( const QString& arg1 )
{
	UNREFERENCED_PARAMETER( arg1 );

	for ( int i = 0; i < ui->tblProcessList->rowCount( ); i++ )
	{
		QString exeName = ui->tblProcessList->item( i, 0 )->text( );

		if ( !exeName.contains( ui->txtFilter->text( ), Qt::CaseInsensitive ) )
		{
			ui->tblProcessList->hideRow( i );
		} else {
			ui->tblProcessList->showRow( i );
		}
	}
}

void DlgProcessList::on_cmdSelect_clicked( )
{
	QModelIndexList indeces = ui->tblProcessList->selectionModel( )->selectedRows( 0 );

	if ( indeces.count( ) <= 0 )
	{
		QMessageBox::critical( this, "Error", "Please select a process" );
		return;
	}

	QTableWidgetItem* pNameItem	= ui->tblProcessList->item( indeces.at( 0 ).row( ), 0 );
	QTableWidgetItem* pPidItem	= ui->tblProcessList->item( indeces.at( 0 ).row( ), 1 );

	this->m_ProcessName = pNameItem->text( );

	bool ok;
	this->m_ProcessId = pPidItem->text( ).toInt( &ok );

	accept( );
}

void DlgProcessList::on_cmdCancel_clicked( )
{
	reject( );
}
