#ifndef CMEMDIALOG_H
#define CMEMDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QString>

namespace Ui {
class MemDialog;
}


#define MAX_RECORDS 50
//Memory structure for saving channel record information
struct  tMem_Record
{
	tMem_Record()
	{
		MemName = "";
		RxCenterFrequency = 0;
		dBStepIndex = 0;
		dBMaxIndex = 0;
		FftAve = 0;
		FftRate = 0;
		DemodMode = 0;
		Volume = 0;
		RfGain = 0;
		AudioCompressionIndex = 0;
		VideoCompressionIndex = 0;
		RxSpanFreq = 0;
		Offset = 0;
		SquelchValue = 0;
		AgcThresh = 0;
		AgcDecay = 0;
		HiCut = 0;
		LowCut = 0;
		Symetric = true;
		AudioFilter = false;
	}
	QString MemName;
	qint64 RxCenterFrequency;
	int dBStepIndex;
	int dBMaxIndex;
	int FftAve;
	int FftRate;
	int DemodMode;
	int Volume;
	int RfGain;
	int AudioCompressionIndex;
	int VideoCompressionIndex;
	int RxSpanFreq;
	int Offset;
	int SquelchValue;
	int AgcThresh;
	int AgcDecay;
	int HiCut;
	int LowCut;
	bool Symetric;
	bool AudioFilter;
};


class CMemDialog : public QDialog
{
	Q_OBJECT

public:
	explicit CMemDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
	~CMemDialog();
	void Init(QString FilePath);
	QString GetFilePath(){return m_FilePath;}
	void AddRecord(tMem_Record Record, bool Edit);
	void UpdateRecord(tMem_Record Record);
	void GetRecord(tMem_Record& Record);

public slots:
	void OnNewItem();
	void OnUpdateSelected();
	void OnRemoveSelected();
	void OnLoadFileSelect();
	void OnSaveAsFileSelect();
	void OnItemDoubleClicked(QTableWidgetItem * item);
	void OnContextMenuRequest(const QPoint& pos);
	void OnSortClicked(int col);
	void OnCellChanged(int row, int col);


signals:
	void SetCurrentSettingMemory();
	void GetCurrentSetting(bool update);

private:
	void SaveMemoryFile();
	tMem_Record GetRecord(int Index){ return m_pDataBase[Index]; }
	QString GetFrequencyString(qint64 freq);
	void UpdateTable();

	Ui::MemDialog *ui;
	QStringList m_TableHeader;
	tMem_Record* m_pDataBase;
	QString m_FilePath;
	QString m_Str;
	bool m_NeedToSave;
	bool m_SortToggle;
};

#endif // CMEMDIALOG_H
