#include "gameinfodialog.h"
#include "ui_gameinfodialog.h"

#include <QDebug>
#include <QMessageBox>
#include <QFileInfo>

#include "gameinfo.h"
#include "globals.h"
#include "settings.h"
#include "sysframe.h"
#include "syswininput.h"
#include "htmlrf.h"
#include "htmlfmt.h"


/* Implementation of the game information enumerator callback interface.
 * See tads3/gameinfo.h for details.
 */
class QTadsGameInfoEnum: public CTadsGameInfo_enum
{
  public:
	QString gameName;
	QString headline;
	QString byLine;
	QString htmlByLine;
	QString email;
	QString desc;
	QString htmlDesc;
	QString version;
	QString published;
	QString date;
	QString lang;
	QString series;
	QString seriesNumber;
	QString genre;
	QString forgiveness;
	QString license;
	QString copyRules;
	QString ifid;

	virtual void
	tads_enum_game_info( const char* name, const char* val )
	{
		const QString& valStr = QString::fromUtf8(val);
		const QString& nameStr = QString::fromUtf8(name).toLower();
		if (nameStr == QString::fromAscii("name")) {
			this->gameName = QString::fromAscii("<b><center><font size=\"+1\">") + Qt::escape(valStr)
							 + QString::fromAscii("</font></center></b><p>");
		} else if (nameStr == QString::fromAscii("headline")) {
			this->headline = QString::fromAscii("<center>") + Qt::escape(valStr)
							 + QString::fromAscii("</center><p>");
		} else if (nameStr == QString::fromAscii("byline")) {
			this->byLine = QString::fromAscii("<i><center>") + Qt::escape(valStr)
						   + QString::fromAscii("</center></i><p>");
		} else if (nameStr == QString::fromAscii("htmlbyline")) {
			this->htmlByLine = QString::fromAscii("<i><center>") + valStr + QString::fromAscii("</center></i><p>");
		} else if (nameStr == QString::fromAscii("authoremail")) {
			this->email = valStr;
		} else if (nameStr == QString::fromAscii("desc")) {
			this->desc = Qt::escape(valStr).replace(QString::fromAscii("\\n"), QString::fromAscii("<p>"));
		} else if (nameStr == QString::fromAscii("htmldesc")) {
			this->htmlDesc = valStr;
		} else if (nameStr == QString::fromAscii("version")) {
			this->version = valStr;
		} else if (nameStr == QString::fromAscii("firstpublished")) {
			this->published = valStr;
		} else if (nameStr == QString::fromAscii("releasedate")) {
			this->date = valStr;
		} else if (nameStr == QString::fromAscii("language")) {
			this->lang = valStr;
		} else if (nameStr == QString::fromAscii("series")) {
			this->series = valStr;
		} else if (nameStr == QString::fromAscii("seriesnumber")) {
			this->seriesNumber = valStr;
		} else if (nameStr == QString::fromAscii("genre")) {
			this->genre = valStr;
		} else if (nameStr == QString::fromAscii("forgiveness")) {
			this->forgiveness = valStr;
		} else if (nameStr == QString::fromAscii("licensetype")) {
			this->license = valStr;
		} else if (nameStr == QString::fromAscii("copyingrules")) {
			this->copyRules = valStr;
		} else if (nameStr == QString::fromAscii("ifid")) {
			this->ifid = valStr;
		}
	}
};


static void
insertTableRow( QTableWidget* table, const QString& text1, const QString& text2 )
{
	table->insertRow(table->rowCount());
	QTableWidgetItem* item = new QTableWidgetItem(text1);
	item->setFlags(Qt::ItemIsEnabled);
	table->setItem(table->rowCount() - 1, 0, item);
	item = new QTableWidgetItem(text2);
	item->setFlags(Qt::ItemIsEnabled);
	table->setItem(table->rowCount() - 1, 1, item);
}


static QImage
loadCoverArtImage()
{
	CHtmlResFinder* resFinder = qFrame->gameWindow()->get_formatter()->get_res_finder();

	// Look for a cover art resource. We try four different resource names.
	// The first two (PNG and JPG with a ".system/" prefix) are defined in the
	// current cover art standard.  The other two were defined in the older
	// standard which did not use a prefix.
	QByteArray coverArtResName;
	bool coverArtFound = false;
	const char coverArtPaths[][21] = {".system/CoverArt.png", ".system/CoverArt.jpg",
									  "CoverArt.png", "CoverArt.jpg"};
	for (int i = 0; i < 4 and not coverArtFound; ++i) {
		coverArtResName = coverArtPaths[i];
		if (resFinder->resfile_exists(coverArtResName.constData(), coverArtResName.length())) {
			coverArtFound = true;
		}
	}

	if (not coverArtFound) {
		return QImage();
	}

	CStringBuf strBuf;
	unsigned long offset;
	unsigned long size;
	resFinder->get_file_info(&strBuf, coverArtResName.constData(), coverArtResName.length(),
							 &offset, &size);

	// Check if the file exists and is readable.
	QFileInfo inf(QString::fromLocal8Bit(strBuf.get()));
	if (not inf.exists() or not inf.isReadable()) {
		qWarning() << "ERROR:" << inf.filePath() << "doesn't exist or is unreadable";
		return QImage();
	}

	// Open the file and seek to the specified position.
	QFile file(inf.filePath());
	if (not file.open(QIODevice::ReadOnly)) {
		qWarning() << "ERROR: Can't open file" << inf.filePath();
		return QImage();
	}
	if (not file.seek(offset)) {
		qWarning() << "ERROR: Can't seek in file" << inf.filePath();
		file.close();
		return QImage();
	}

	// Load the image data.
	const QByteArray& data(file.read(size));
	file.close();
	if (data.isEmpty() or static_cast<unsigned long>(data.size()) < size) {
		qWarning() << "ERROR: Could not read" << size << "bytes from file" << inf.filePath();
		return QImage();
	}
	QImage image;
	if (not image.loadFromData(data)) {
		qWarning() << "ERROR: Could not parse image data";
		return QImage();
	}

	// If we got here, all went well.  Return the image scaled to a
	// 200 pixels width if it's too large.  Otherwise, return it as-is.
	if (image.width() > 200) {
		Qt::TransformationMode mode = qFrame->settings()->useSmoothScaling ?
									  Qt::SmoothTransformation : Qt::FastTransformation;
		return image.scaledToWidth(200, mode);
	} else {
		return image;
	}
}


GameInfoDialog::GameInfoDialog( const QByteArray& fname, QWidget* parent )
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint), ui(new Ui::GameInfoDialog)
{
    ui->setupUi(this);

	CTadsGameInfo info;
	QTadsGameInfoEnum cb;

	// Try to load the cover art.  If there is one, insert it into the text
	// browser as the "CoverArt" resource.
	const QImage& image = loadCoverArtImage();
	if (not image.isNull()) {
		ui->description->document()->addResource(QTextDocument::ImageResource,
												 QUrl(QString::fromAscii("CoverArt")), image);
		this->resize(this->width(), this->height() + image.height());
	}

	info.read_from_file(fname.constData());
	info.enum_values(&cb);

	// Fill out the description.
	QString tmp;
	if (not image.isNull()) {
		tmp += QString::fromAscii("<center><img src=\"CoverArt\"></center><p>");
	}
	tmp += cb.gameName + cb.headline + (cb.htmlByLine.isEmpty() ? cb.byLine : cb.htmlByLine)
		   + (cb.htmlDesc.isEmpty() ? cb.desc : cb.htmlDesc);
	ui->description->setHtml(tmp);

	// Fill out the table.
	ui->table->setColumnCount(2);
	if (not cb.genre.isEmpty()) {
		insertTableRow(ui->table, tr("Genre"), cb.genre);
	}

	if (not cb.version.isEmpty()) {
		insertTableRow(ui->table, tr("Version"), cb.version);
	}

	if (not cb.forgiveness.isEmpty()) {
		insertTableRow(ui->table, tr("Forgiveness"), cb.forgiveness);
	}

	if (not cb.series.isEmpty()) {
		insertTableRow(ui->table, tr("Series"), cb.series);
	}

	if (not cb.seriesNumber.isEmpty()) {
		insertTableRow(ui->table, tr("Series Number"), cb.seriesNumber);
	}

	if (not cb.date.isEmpty()) {
		insertTableRow(ui->table, tr("Date"), cb.date);
	}

	if (not cb.published.isEmpty()) {
		insertTableRow(ui->table, tr("First Published"), cb.published);
	}

	if (not cb.email.isEmpty()) {
		insertTableRow(ui->table, tr("Author email"), cb.email);
	}

	if (not cb.lang.isEmpty()) {
		insertTableRow(ui->table, tr("Language"), cb.lang);
	}

	if (not cb.license.isEmpty()) {
		insertTableRow(ui->table, tr("License Type"), cb.license);
	}

	if (not cb.copyRules.isEmpty()) {
		insertTableRow(ui->table, tr("Copying Rules"), cb.copyRules);
	}

	if (not cb.ifid.isEmpty()) {
		insertTableRow(ui->table, tr("IFID"), cb.ifid);
	}

	ui->table->resizeColumnsToContents();
	ui->table->resizeRowsToContents();
	ui->table->setShowGrid(false);
	// There's no point in having the tableview widget be any higher than the
	// sum of all rows.  This will also make sure that the widget is not shown
	// at all in case we have zero rows.
	ui->table->setMaximumHeight((ui->table->rowCount() + 1) * ui->table->rowHeight(0));
}


GameInfoDialog::~GameInfoDialog()
{
    delete ui;
}


bool
GameInfoDialog::gameHasMetaInfo( const QByteArray& fname )
{
	CTadsGameInfo info;
	return info.read_from_file(fname.constData());
}
