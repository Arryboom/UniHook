#include "DlgMemory.h"
#include "ui_DlgMemory.h"

#include "MainWindow.h"

#include <QFileInfo>
#include <psapi.h>

DlgMemory::DlgMemory( QWidget* pParent ) : QDialog( pParent ), ui( new Ui::DlgMemory )
{
	ui->setupUi( this );

	/*
	MainWindow* pMainWindow = reinterpret_cast<MainWindow*>( pParent );

	if ( pMainWindow->m_hProcess == 0 || pMainWindow->m_Regions.size( ) == 0 )
	{
		ui->tblRegions->setVisible( false );
		ui->group->setTitle( "Memory Regions (Failed to dump)" );
	}

	CreateHeader( );
	RefreshTable( );
	JumpToOrigin( );
	*/
}

void DlgMemory::CreateHeader( )
{
	ui->tblRegions->setSelectionBehavior( QAbstractItemView::SelectRows );
	ui->tblRegions->horizontalHeader( )->setStretchLastSection( true );
	ui->tblRegions->horizontalHeader( )->setDefaultAlignment( Qt::AlignLeft );
	ui->tblRegions->verticalHeader( )->setDefaultSectionSize( ui->tblRegions->verticalHeader( )->fontMetrics( ).height( ) + 2 );
	ui->tblRegions->verticalHeader( )->hide( );
	ui->tblRegions->setSortingEnabled( false );
	ui->tblRegions->setHorizontalHeaderItem( 0, new QTableWidgetItem( "Address" ) );
	ui->tblRegions->setHorizontalHeaderItem( 1, new QTableWidgetItem( "Mapped File" ) );
	ui->tblRegions->setHorizontalHeaderItem( 2, new QTableWidgetItem( "Section" ) );
	ui->tblRegions->setHorizontalHeaderItem( 3, new QTableWidgetItem( "Size" ) );
	ui->tblRegions->setHorizontalHeaderItem( 4, new QTableWidgetItem( "Protection" ) );
	ui->tblRegions->setHorizontalHeaderItem( 5, new QTableWidgetItem( "Type" ) );
}

void DlgMemory::RefreshTable( )
{
	/*
	Context* pContext = Context::GetInstance( );

	ui->tblRegions->setRowCount( 0 );

	foreach ( MemoryRegion* pRegion, pContext->m_ProcessContext.m_Regions )
	{
		if ( !pRegion->m_BaseAddress )
			continue;

		int row = ui->tblRegions->rowCount( );
		ui->tblRegions->insertRow( ui->tblRegions->rowCount( ) );

		ui->tblRegions->setItem( row, 0, new QTableWidgetItem( QString::number( pRegion->m_BaseAddress, 16 ) ) );

		QTableWidgetItem* pAddressItem = ui->tblRegions->item( row, 0 );

		if ( pAddressItem )
			pAddressItem->setTextAlignment( Qt::AlignRight | Qt::AlignVCenter );

		ui->tblRegions->setItem( row, 1, new QTableWidgetItem( pRegion->m_ModuleName ) );
        ui->tblRegions->setItem( row, 2, new QTableWidgetItem( pRegion->m_SectionName ) );
		ui->tblRegions->setItem( row, 3, new QTableWidgetItem( QString::number( pRegion->m_RegionSize, 16 ).toUpper( ) ) );
		ui->tblRegions->setItem( row, 4, new QTableWidgetItem( GetPageProtections( pRegion->m_Protect ) ) );
		ui->tblRegions->setItem( row, 5, new QTableWidgetItem( GetPageType( pRegion->m_Type ) ) );
	}

	ui->tblRegions->horizontalHeader( )->setSectionResizeMode( QHeaderView::ResizeToContents );
	ui->tblRegions->verticalHeader( )->setSectionResizeMode( QHeaderView::ResizeToContents );
	*/
}

void DlgMemory::JumpToOrigin( )
{
	/*
	Context* pContext = Context::GetInstance( );

	for ( int i = ui->tblRegions->rowCount( ); i > 0; i-- )
	{
		QTableWidgetItem* pItem = ui->tblRegions->item( i, 1 );

		if ( !pItem )
			continue;

		if ( pItem->text( ) == pContext->m_ProcessContext.m_ProcessName )
		{
			ui->tblRegions->scrollToItem( pItem );
			ui->tblRegions->selectRow( i );
		}
	}
	*/
}

QString DlgMemory::GetPageGuardNoCache( DWORD dwProtect )
{
	int guard = 0, nocache = 0;

	if ( dwProtect & PAGE_NOCACHE )
		nocache = 1;
	if ( dwProtect & PAGE_GUARD )
		guard   = 1;

	QString ret;

	if ( guard )
		ret += "PAGE_GUARD ";

	if ( nocache )
		ret += "PAGE_NOCACHE";

	return "";
}

QString DlgMemory::GetPageProtections( DWORD dwProtect )
{
   dwProtect &= ~( PAGE_GUARD | PAGE_NOCACHE );

	switch ( dwProtect )
	{
	case PAGE_READONLY:
		return "READONLY";
		break;
	case PAGE_READWRITE:
		return "READWRITE";
		break;
	case PAGE_WRITECOPY:
		return "WRITECOPY";
		break;
	case PAGE_EXECUTE:
		return "EXECUTE";
		break;
	case PAGE_EXECUTE_READ:
		return "EXECUTE_READ";
		break;
	case PAGE_EXECUTE_READWRITE:
		return "EXECUTE_READWRITE";
		break;
	case PAGE_EXECUTE_WRITECOPY:
		return "EXECUTE_WRITECOPY";
		break;
	}

	return "";
}

QString DlgMemory::GetPageType( DWORD dwType )
{
	switch ( dwType )
	{
	case MEM_IMAGE:
		return "Code Module";
		break;
	case MEM_MAPPED:
		return "Mapped";
		break;
	case MEM_PRIVATE:
		return "Private";
	}

	return "";
}

QString DlgMemory::GetPageState( DWORD dwState )
{
	switch ( dwState )
	{
	case MEM_COMMIT:
		return "Committed";
		break;
	case MEM_RESERVE:
		return "Reserved";
		break;
	case MEM_FREE:
		return "Free";
		break;
	}

	return "";
}


DlgMemory::~DlgMemory( )
{
	delete ui;
}
