#include "memdialog.h"
#include "ui_memdialog.h"
#include <QSettings>
#include <QDebug>
#include <QFileInfo>
#include <QFileDialog>

//convert index to demod string
static const char* MODE_TABLE[] = { "LSB", "USB", "DSB", "CWL", "CWU", "FM", "AM", "SAM", "WFM", "WAM"};


CMemDialog::CMemDialog(QWidget *parent, Qt::WindowFlags f) :
	QDialog(parent,f),
	ui(new Ui::MemDialog)
{
	ui->setupUi(this);
	//create temporary storage for memory file database
	m_pDataBase = new tMem_Record[MAX_RECORDS];
	m_FilePath = "";


	//connect signal for double click on header for sorting
	ui->tableWidgetMem->setContextMenuPolicy(Qt::CustomContextMenu);
	connect( ui->tableWidgetMem, SIGNAL( customContextMenuRequested(const QPoint &) ),
			 this, SLOT( OnContextMenuRequest(const QPoint &) ) );

	//create Table Widget
	ui->tableWidgetMem->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->tableWidgetMem->verticalHeader()->setVisible(false);
	ui->tableWidgetMem->horizontalHeader()->setStretchLastSection(true);
	ui->tableWidgetMem->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->tableWidgetMem->resizeRowsToContents();
	ui->tableWidgetMem->setAlternatingRowColors(true);
	QFont fnt = ui->tableWidgetMem->horizontalHeader()->font();
	fnt.setBold(true);
	ui->tableWidgetMem->horizontalHeader()->setFont(fnt);
	ui->tableWidgetMem->horizontalHeader()->setHighlightSections( false);
	ui->tableWidgetMem->horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter|Qt::AlignVCenter );
	connect( ui->tableWidgetMem->horizontalHeader(), SIGNAL( sectionDoubleClicked(int) ),
			 this, SLOT( OnSortClicked(int) ) );

	m_NeedToSave = false;
	m_SortToggle = false;
}

CMemDialog::~CMemDialog()
{
	if(m_NeedToSave)
		SaveMemoryFile();
	if(m_pDataBase)
		delete[] m_pDataBase;
	delete ui;
}


//Called to bring up File dialog to load a different .ini file
void CMemDialog::OnLoadFileSelect()
{
QString str = QFileDialog::getOpenFileName(this,tr("Select Memory File to Load"),m_FilePath,tr("Memory files (*.ini)"));
	if(str!="")
		m_FilePath = str;
	Init(m_FilePath);
}

//Called to bring up File dialog to change name of .ini file to save
void CMemDialog::OnSaveAsFileSelect()
{
QString str = QFileDialog::getSaveFileName(this,tr("Select New Memory File Name"),m_FilePath,tr("Memory files (*.ini)"));
	if(str!="")
	{
		m_FilePath = str;
		m_NeedToSave = true;
		QFileInfo info(m_FilePath);
		this->setWindowTitle("Memory File = " + info.fileName());
	}
}

//save data from database array into .ini file
void CMemDialog::SaveMemoryFile()
{
	int NumChannels = ui->tableWidgetMem->rowCount();
	QSettings Memories(m_FilePath, QSettings::IniFormat);
	if(NumChannels==0)
	{
		Memories.clear();
	}
	else if(NumChannels>0)
	{
		Memories.clear();
		Memories.beginWriteArray("Channels");
		for(int i=0; i<NumChannels; i++)
		{
			Memories.setArrayIndex(i);
			Memories.setValue("MemName",m_pDataBase[i].MemName);
			Memories.setValue("RxCenterFrequency",m_pDataBase[i].RxCenterFrequency);
			Memories.setValue("dBStepIndex",m_pDataBase[i].dBStepIndex);
			Memories.setValue("dBMaxIndex",m_pDataBase[i].dBMaxIndex);
			Memories.setValue("FftAve",m_pDataBase[i].FftAve);
			Memories.setValue("FftRate",m_pDataBase[i].FftRate);
			Memories.setValue("DemodMode",m_pDataBase[i].DemodMode);
			Memories.setValue("Volume",m_pDataBase[i].Volume);
			Memories.setValue("RfGain",m_pDataBase[i].RfGain);
			Memories.setValue("AudioCompressionIndex",m_pDataBase[i].AudioCompressionIndex);
			Memories.setValue("VideoCompressionIndex",m_pDataBase[i].VideoCompressionIndex);
			Memories.setValue("RxSpanFreq",m_pDataBase[i].RxSpanFreq);
			Memories.setValue("Offset",m_pDataBase[i].Offset);
			Memories.setValue("SquelchValue",m_pDataBase[i].SquelchValue);
			Memories.setValue("AgcThresh",m_pDataBase[i].AgcThresh);
			Memories.setValue("AgcDecay",m_pDataBase[i].AgcDecay);
			Memories.setValue("HiCut",m_pDataBase[i].HiCut);
			Memories.setValue("LowCut",m_pDataBase[i].LowCut);
			Memories.setValue("Symetric",m_pDataBase[i].Symetric);
			Memories.setValue("AudioFilter",m_pDataBase[i].AudioFilter);
		}
		Memories.endArray();
	}
}
//Called when Description field is to be edited
void CMemDialog::OnContextMenuRequest(const QPoint& pos) // this is a slot
{
	Q_UNUSED(pos);
	int col = ui->tableWidgetMem->currentColumn();
	if(col!=1)
		return;	//if not editable column
	ui->tableWidgetMem->editItem(ui->tableWidgetMem->currentItem());
	m_NeedToSave = true;
}

//Called upon completion of description field edit to update m_pDataBase array
void CMemDialog::OnCellChanged(int row, int col)
{
	if(1 == col)
	{
		QTableWidgetItem * Item = ui->tableWidgetMem->item(row,col);
//		qDebug()<<Item->text() <<row << col;
		m_pDataBase[row].MemName = Item->text();
	}
}

//Called to sort table by description string
void CMemDialog::OnSortClicked(int col)
{
	if(col==1)
	{
		int NumChannels = ui->tableWidgetMem->rowCount();
		//create temporary copy of memory file database
		tMem_Record* pTmpDataBase = new tMem_Record[MAX_RECORDS];
		for(int i = 0; i<NumChannels; i++)
			pTmpDataBase[i] = m_pDataBase[i];
		if(m_SortToggle)
		{
			ui->tableWidgetMem->sortItems(1,Qt::DescendingOrder);
			m_SortToggle = false;
			ui->tableWidgetMem->horizontalHeaderItem(1)->setText("Description (Decending)");
		}
		else
		{
			ui->tableWidgetMem->sortItems(1,Qt::AscendingOrder);
			m_SortToggle = true;
			ui->tableWidgetMem->horizontalHeaderItem(1)->setText("Description (Ascending)");
		}
		for(int i = 0; i<NumChannels; i++)
		{
			QTableWidgetItem* Item = ui->tableWidgetMem->item(i,1);
			m_pDataBase[i] = pTmpDataBase[Item->data(Qt::UserRole).toInt()];
			Item->setData(Qt::UserRole ,i);
		}
		m_NeedToSave = true;
	}
}

// Initialize m_pDataBase from databaase file specified by FilePath
void CMemDialog::Init(QString FilePath)
{
	m_FilePath = FilePath;
	QFileInfo info(m_FilePath);
	this->setWindowTitle("Memory File = " + info.fileName());
	//setup table widget
	ui->tableWidgetMem->clear();
	ui->tableWidgetMem->setRowCount(0);
	ui->tableWidgetMem->setColumnCount(2);
	QFont fnt = ui->tableWidgetMem->horizontalHeader()->font();
	fnt.setBold(true);
	fnt.setUnderline(true);
	ui->tableWidgetMem->horizontalHeader()->setFont(fnt);
	m_TableHeader<<"Frequency/Mode"<<"Description";
	ui->tableWidgetMem->setHorizontalHeaderLabels(m_TableHeader);
	ui->tableWidgetMem->horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter|Qt::AlignVCenter );

	//read ini file into table and database array
	QSettings Memories(m_FilePath, QSettings::IniFormat);
	int NumChannels = Memories.beginReadArray("Channels");
	tMem_Record Tmp;
	for(int i=0; i<NumChannels; i++)
	{
		Memories.setArrayIndex(i);
		Tmp.MemName = Memories.value("MemName","").toString();
		Tmp.RxCenterFrequency = Memories.value("RxCenterFrequency", 10000000).toLongLong();
		Tmp.dBStepIndex = Memories.value("dBStepIndex", 0).toInt();
		Tmp.dBMaxIndex =  Memories.value("dBMaxIndex", 0).toInt();
		Tmp.FftAve = Memories.value("FftAve", 1).toInt();
		Tmp.FftRate =  Memories.value("FftRate", 1).toInt();
		Tmp.DemodMode = Memories.value("DemodMode", 0).toInt();
		Tmp.Volume =  Memories.value("Volume", 0).toInt();
		Tmp.RfGain = Memories.value("RfGain", 0).toInt();
		Tmp.AudioCompressionIndex = Memories.value("AudioCompressionIndex", 0).toInt();
		Tmp.VideoCompressionIndex = Memories.value("VideoCompressionIndex", 0).toInt();
		Tmp.RxSpanFreq =  Memories.value("RxSpanFreq",10000).toInt();
		Tmp.Offset =  Memories.value("Offset", 0).toInt();
		Tmp.SquelchValue = Memories.value("SquelchValue", 0).toInt();
		Tmp.AgcThresh =  Memories.value("AgcThresh", 0).toInt();
		Tmp.AgcDecay =  Memories.value("AgcDecay", 0).toInt();
		Tmp.HiCut =  Memories.value("HiCut", 1000).toInt();
		Tmp.LowCut = Memories.value("LowCut", -1000).toInt();
		Tmp.Symetric = Memories.value("Symetric", true).toBool();
		Tmp.AudioFilter = Memories.value("AudioFilter", false).toBool();
		AddRecord(Tmp, false);
	}
//	qDebug()<<"File = "<<m_FilePath<<"Num entries = "<<m_NumChannels;
}

//called when get current settings button is pressed
// so then signal parent(Main) to send its current settings back to us using AddRecord() call
void CMemDialog::OnNewItem()
{
	emit GetCurrentSetting(false);	//signal parent to get current settings and call AddRecord()
}

//called when record is selected to change current prgram settings to this Records data
//it signal parent(Main) to call GetRecord() to get memory data
void CMemDialog::OnItemDoubleClicked(QTableWidgetItem * item)
{
Q_UNUSED(item);
	emit SetCurrentSettingMemory();	//signal parent to get current settings and call AddRecord()
}

//Called by Main to get selected record data
void CMemDialog::GetRecord(tMem_Record& Record)
{
	int index = ui->tableWidgetMem->currentRow();
	if(index<0)
		return;		//no row selected
	Record = m_pDataBase[index];
//	qDebug()<<"GetRecord";
}


//Add new record to next available at end of m_pDataBase
void CMemDialog::AddRecord(tMem_Record Record, bool Edit)
{
	int index = ui->tableWidgetMem->rowCount();	//get index to next free position in list
	if( (index<0) || (index>=MAX_RECORDS) )
		return;
	ui->tableWidgetMem->setRowCount(index+1);
	m_pDataBase[index] = Record;	//put in DataBase array
	QTableWidgetItem * newitem0 = new QTableWidgetItem();
	newitem0->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
	QTableWidgetItem * newitem1 = new QTableWidgetItem();
	newitem1->setTextAlignment(Qt::AlignCenter|Qt::AlignVCenter);

	m_Str = GetFrequencyString(m_pDataBase[index].RxCenterFrequency);
	m_Str += MODE_TABLE[m_pDataBase[index].DemodMode];
	newitem0->setText(m_Str);
	ui->tableWidgetMem->setItem(index, 0, newitem0);

	newitem1->setText(m_pDataBase[index].MemName);
	ui->tableWidgetMem->setItem(index, 1, newitem1);
	newitem1->setData( Qt::UserRole, index );

	if(Edit)
	{
		ui->tableWidgetMem->setCurrentCell(index, 1);
		ui->tableWidgetMem->editItem(ui->tableWidgetMem->currentItem());
		m_NeedToSave = true;
	}
//	qDebug()<<"Add New Record"<<m_pDataBase[index].DemodMode;
}


//Called when Update button is pressed to signal parent(Main) to
// send its current settings to update the selected record using UpdateRecord() call
void CMemDialog::OnUpdateSelected()
{
	int index = ui->tableWidgetMem->currentRow();
	if(index<0)
		return;		//no row selected
	emit GetCurrentSetting(true);	//signal parent to get current settings and call AddRecord()
}

void CMemDialog::UpdateRecord(tMem_Record Record)
{
	int index = ui->tableWidgetMem->currentRow();
	if(index<0)
		return;		//no row selected
	m_pDataBase[index] = Record;
	QTableWidgetItem* Item = ui->tableWidgetMem->item(index,0);
	m_Str = GetFrequencyString(m_pDataBase[index].RxCenterFrequency);
	m_Str += MODE_TABLE[m_pDataBase[index].DemodMode];
	Item->setText(m_Str);
	ui->tableWidgetMem->setItem(index, 0, Item);
	m_NeedToSave = true;
//	qDebug()<<"Update Record";
}

void CMemDialog::OnRemoveSelected()
{
	int index = ui->tableWidgetMem->currentRow();
	if(index<0)
		return;		//no row selected
	int NumChannels = ui->tableWidgetMem->rowCount();
	ui->tableWidgetMem->removeRow(index);
	for(int i = index; i<(NumChannels-1); i++)
		m_pDataBase[i] = m_pDataBase[i+1];	//move up remaing records
	NumChannels = ui->tableWidgetMem->rowCount();
	for(int i = 0; i<NumChannels; i++)
	{
		QTableWidgetItem* Item = ui->tableWidgetMem->item(i,1);
		Item->setData( Qt::UserRole, i );
	}
	m_NeedToSave = true;
//	qDebug()<<"RemoveSelected" << index << m_NumChannels;
}

QString CMemDialog::GetFrequencyString(qint64 freq)
{
double f = (double)freq;
QString tmp;
	if(freq<1000000)
		tmp = QString("%1 kHz ").arg(f/1e3, 0, 'g', 6);
	else if(freq<1000000000)
		tmp = QString("%1 MHz ").arg(f/1e6, 0, 'g', 9);
	else
		tmp = QString("%1 GHz ").arg(f/1e9, 0, 'g', 9);
	return tmp;
}

