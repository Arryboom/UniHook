#ifndef DLGFIND_H
#define DLGFIND_H

#include <QDialog>
#include <QString>

namespace Ui
{
	class DlgFind;
}

class DlgFind : public QDialog
{
	Q_OBJECT

public:
	explicit DlgFind( QWidget* pParent = 0 );
	~DlgFind( );

	QString m_Address;

private slots:
	void on_cmdSearch_clicked( );
	void on_cmdCancel_clicked( );

private:
	Ui::DlgFind* ui;
};

#endif // DLGFIND_H
