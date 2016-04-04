#include "DlgFind.h"
#include "ui_DlgFind.h"

DlgFind::DlgFind( QWidget* pParent ) : QDialog( pParent ), ui( new Ui::DlgFind )
{
	ui->setupUi( this );
}

DlgFind::~DlgFind( )
{
	delete ui;
}

void DlgFind::on_cmdSearch_clicked( )
{
	this->m_Address = ui->txtAddress->text( );
	accept( );
}

void DlgFind::on_cmdCancel_clicked()
{
	reject( );
}
