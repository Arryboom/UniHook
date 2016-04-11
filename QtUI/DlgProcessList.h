#ifndef DLGPROCESSLIST_H
#define DLGPROCESSLIST_H

#include <QDialog>
#include <QString>

#include "windows.h"

namespace Ui
{
	class DlgProcessList;
}

class DlgProcessList : public QDialog
{
	Q_OBJECT

public:
	explicit DlgProcessList( QWidget* pParent = 0 );
	void RefreshProcessList( );
	~DlgProcessList( );

	QString m_ProcessName;
	DWORD m_ProcessId;


private slots:
	void on_txtFilter_textChanged( const QString& arg1 );
	void on_cmdSelect_clicked( );
	void on_cmdCancel_clicked( );

private:
	Ui::DlgProcessList* ui;
};

#endif // DLGPROCESSLIST_H
