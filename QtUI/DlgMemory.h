#ifndef DLGMEMORY_H
#define DLGMEMORY_H

#include <QDialog>

#include <windows.h>

namespace Ui
{
	class DlgMemory;
}

class DlgMemory : public QDialog
{
	Q_OBJECT

public:
	explicit DlgMemory( QWidget* pParent = 0 );

	void CreateHeader( );
	void RefreshTable( );
	void JumpToOrigin( );

	QString GetPageGuardNoCache( DWORD dwProtect );
	QString GetPageProtections( DWORD dwProtect );
	QString GetPageType( DWORD dwType );
	QString GetPageState( DWORD dwState );

	~DlgMemory( );

private:
	Ui::DlgMemory* ui;
};

#endif // DLGMEMORY_H
