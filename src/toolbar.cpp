/*
Toolbar de Resolução
Copyright (C) 2026 victoralexbr <victoralexbr@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "toolbar.h"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <util/config-file.h>

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QCursor>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QWidget>
#include <functional>

namespace {

/* Presets prontos de resolução: muda o Canvas (base) e a Saída (output)
 * de uma vez, igual a configuração manual de "Vídeo" do OBS. */
struct Preset {
	const char *localeKey;
	const char *fallback;
	uint32_t baseWidth;
	uint32_t baseHeight;
	uint32_t outputWidth;
	uint32_t outputHeight;
};

const Preset kPresets[] = {
	{"Toolbar.Preset.1080H", "1080p Horizontal", 1920, 1080, 1920, 1080},
	{"Toolbar.Preset.1080V", "1080p Vertical", 1080, 1920, 1080, 1920},
	{"Toolbar.Preset.720H", "720p Horizontal", 1280, 720, 1280, 720},
	{"Toolbar.Preset.720V", "720p Vertical", 720, 1280, 720, 1280},
};

QString localeText(const char *key, const char *fallback)
{
	const char *value = nullptr;
	if (obs_module_get_string(key, &value) && value && *value)
		return QString::fromUtf8(value);
	return QString::fromUtf8(fallback);
}

/* Link de apoio exibido no "Sobre…" e usado no clique da frase. */
const char kSupportUrl[] = "https://livepix.gg/victoralexbr";
const char kSupportText[] =
	"Se fui \u00FAtil considere ajudar no https://livepix.gg/victoralexbr "
	"com qualquer valor";

/* Avisa que a resolução não pode mudar com saída ativa: o próprio OBS
 * bloqueia (ResetVideo devolve OBS_VIDEO_CURRENTLY_ACTIVE). */
void warnVideoActive(QWidget *parent)
{
	QString title = localeText("Toolbar.Active.Title", "Output is active");
	QString text = localeText(
		"Toolbar.Active.Text",
		"Recording or streaming is active. OBS cannot change the "
		"resolution while an output is running. Stop the output "
		"first, then try again.");
	QMessageBox::warning(parent, title, text);
}

/* obs_frontend_reset_video não reporta o motivo da falha; quando o vídeo
 * não mudou para o pedido, mostramos o erro genérico do plugin. */
void warnApplyFailed(QWidget *parent)
{
	QString title = localeText("Toolbar.Error.Title", "Could not change resolution");
	QString text = localeText(
		"Toolbar.Error.Text",
		"OBS refused the resolution change. Check the video "
		"settings and try again.");
	QMessageBox::warning(parent, title, text);
}

/* Modo de organização da barra: Automático (seguindo o formato da dock),
 * Horizontal ou Empilhado (vertical). Persistido via QSettings. */
enum class LayoutMode { Auto, Horizontal, Vertical };

const char kSettingsOrganization[] = "victoralexbr";
const char kSettingsApplication[] = "easy-aspect-ratio-changer-obs";
const char kSettingsLayoutKey[] = "LayoutMode";

LayoutMode loadLayoutMode()
{
	QSettings settings(kSettingsOrganization, kSettingsApplication);
	const QString value = settings.value(kSettingsLayoutKey, "auto").toString();
	if (value == QLatin1String("horizontal"))
		return LayoutMode::Horizontal;
	if (value == QLatin1String("vertical"))
		return LayoutMode::Vertical;
	return LayoutMode::Auto;
}

void saveLayoutMode(LayoutMode mode)
{
	QSettings settings(kSettingsOrganization, kSettingsApplication);
	QString value;
	switch (mode) {
	case LayoutMode::Horizontal:
		value = "horizontal";
		break;
	case LayoutMode::Vertical:
		value = "vertical";
		break;
	default:
		value = "auto";
		break;
	}
	settings.setValue(kSettingsLayoutKey, value);
	settings.sync();
}

/* Widget topo da dock: organiza a barra na horizontal ou empilhada.
 * Sem Q_OBJECT: nada de moc, só resizeEvent/contextMenuEvent sobrescritos. */
class QuickResWidget : public QWidget {
public:
	explicit QuickResWidget(QWidget *parent = nullptr)
		: QWidget(parent), box_(new QBoxLayout(QBoxLayout::LeftToRight, this)),
		  manualMode_(loadLayoutMode())
	{
		box_->setContentsMargins(6, 3, 6, 3);
		box_->setSpacing(6);
		applyCurrentMode();
	}

	QBoxLayout *box() { return box_; }

	bool isStacked() const
	{
		return box_ && box_->direction() == QBoxLayout::TopToBottom;
	}

	void setManualMode(LayoutMode mode)
	{
		manualMode_ = mode;
		saveLayoutMode(mode);
		applyCurrentMode();
	}

	/* Menu "três pontinhos": layout + Sobre. */
	void showLayoutMenu(const QPoint &globalPos)
	{
		QMenu menu(this);

		QAction *autoAction =
			menu.addAction(localeText("Toolbar.Layout.Auto", "Auto"));
		QAction *horizontalAction = menu.addAction(
			localeText("Toolbar.Layout.Horizontal", "Horizontal"));
		QAction *verticalAction = menu.addAction(
			localeText("Toolbar.Layout.Vertical", "Stacked (vertical)"));

		autoAction->setCheckable(true);
		horizontalAction->setCheckable(true);
		verticalAction->setCheckable(true);

		QActionGroup *group = new QActionGroup(&menu);
		group->setExclusive(true);
		group->addAction(autoAction);
		group->addAction(horizontalAction);
		group->addAction(verticalAction);

		switch (manualMode_) {
		case LayoutMode::Horizontal:
			horizontalAction->setChecked(true);
			break;
		case LayoutMode::Vertical:
			verticalAction->setChecked(true);
			break;
		default:
			autoAction->setChecked(true);
			break;
		}

		QObject::connect(autoAction, &QAction::triggered, this,
				 [this]() { setManualMode(LayoutMode::Auto); });
		QObject::connect(horizontalAction, &QAction::triggered, this,
				 [this]() { setManualMode(LayoutMode::Horizontal); });
		QObject::connect(verticalAction, &QAction::triggered,
				 this,
				 [this]() { setManualMode(LayoutMode::Vertical); });

		/* Última linha do menu: "Sobre…" com janela de apoio. */
		menu.addSeparator();
		QAction *aboutAction =
			menu.addAction(localeText("Toolbar.About.Menu", "About…"));
		QObject::connect(aboutAction, &QAction::triggered, this,
				 [this]() { showAboutDialog(); });

		menu.exec(globalPos);
	}

	/* Janela "Sobre": frase de apoio; clicar na frase abre o link. */
	void showAboutDialog()
	{
		const QUrl supportUrl(QStringLiteral(kSupportUrl));
		const QString phrase = localeText("Toolbar.About.Text", kSupportText);
		const QString html =
			QString("<a href=\"%1\">%2</a>")
				.arg(supportUrl.toString(), phrase.toHtmlEscaped());

		QMessageBox box(this);
		box.setWindowTitle(localeText("Toolbar.About.Title", "About"));
		box.setIcon(QMessageBox::Information);
		box.setTextFormat(Qt::RichText);
		box.setText(html);
		box.setStandardButtons(QMessageBox::Close);
		QObject::connect(&box, &QMessageBox::linkActivated, &box,
				 [](const QString &link) {
					 QDesktopServices::openUrl(QUrl(link));
				 });
		box.exec();
	}

protected:
	void resizeEvent(QResizeEvent *event) override
	{
		if (manualMode_ == LayoutMode::Auto)
			applyCurrentMode();
		QWidget::resizeEvent(event);
	}

	void contextMenuEvent(QContextMenuEvent *event) override
	{
		showLayoutMenu(event->globalPos());
		QWidget::contextMenuEvent(event);
	}

private:
	/* Em Automático: dock alta e estreita (apoiada na lateral) = empilhada;
	 * dock larga (rodapé/topo) = horizontal. */
	bool isNarrow() const { return size().width() < size().height(); }

	void applyCurrentMode()
	{
		if (!box_)
			return;
		switch (manualMode_) {
		case LayoutMode::Horizontal:
			box_->setDirection(QBoxLayout::LeftToRight);
			break;
		case LayoutMode::Vertical:
			box_->setDirection(QBoxLayout::TopToBottom);
			break;
		default:
			box_->setDirection(isNarrow() ? QBoxLayout::TopToBottom
						      : QBoxLayout::LeftToRight);
			break;
		}
	}

	QBoxLayout *box_ = nullptr;
	LayoutMode manualMode_ = LayoutMode::Auto;
};

/* Troca horizontal <-> vertical mantendo a "qualidade" (a menor dimensão).
 * Ex.: 2560x1440 -> 1440x2560; funciona com qualquer resolução. */
struct obs_video_info swappedVideo(const struct obs_video_info &ovi)
{
	struct obs_video_info swapped = ovi;
	swapped.base_width = ovi.base_height;
	swapped.base_height = ovi.base_width;
	swapped.output_width = ovi.output_height;
	swapped.output_height = ovi.output_width;
	return swapped;
}

/* Aplica a resolução exatamente pela mesma via do OBS: grava Canvas e Saída
 * na configuração do perfil e pede obs_frontend_reset_video à interface.
 * O ResetVideo do OBS (frontend/widgets/OBSBasic.cpp) lê a config, chama
 * obs_reset_video e, no sucesso, Redimensiona e centraliza a pré-view
 * (efeito "ajustar à janela") — chamar obs_reset_video direto evade esse
 * passo e deixa o zoom do preview desatualizado. */
void applyVideo(QWidget *widget, const std::function<void()> &refresh, uint32_t baseWidth,
		uint32_t baseHeight, uint32_t outputWidth, uint32_t outputHeight)
{
	if (obs_video_active()) {
		warnVideoActive(widget);
		return;
	}

	config_t *config = obs_frontend_get_profile_config();
	if (!config) {
		warnApplyFailed(widget);
		return;
	}

	config_set_uint(config, "Video", "BaseCX", baseWidth);
	config_set_uint(config, "Video", "BaseCY", baseHeight);
	config_set_uint(config, "Video", "OutputCX", outputWidth);
	config_set_uint(config, "Video", "OutputCY", outputHeight);
	config_save_safe(config, "tmp", nullptr);

	obs_frontend_reset_video();

	/* obs_frontend_reset_video é void: confere o vídeo real antes de
	 * decidir entre sucesso e erro. */
	struct obs_video_info ovi;
	bool ok = obs_get_video_info(&ovi) && ovi.base_width == baseWidth &&
		  ovi.base_height == baseHeight && ovi.output_width == outputWidth &&
		  ovi.output_height == outputHeight;
	if (!ok && !obs_video_active()) {
		warnApplyFailed(widget);
		return;
	}

	refresh();
	/* garante visor correto caso o OBS recalcule depois */
	QTimer::singleShot(600, widget, [refresh]() { refresh(); });
}

} // namespace

void quickres_create_dock()
{
	QuickResWidget *widget = new QuickResWidget();

	QLabel *label = new QLabel(QStringLiteral("\u2014"), widget);
	/* texto inicial: um traço até o primeiro refresh */
	label->setMinimumWidth(84);
	label->setAlignment(Qt::AlignCenter);

	QComboBox *combo = new QComboBox(widget);
	combo->setToolTip(localeText("Toolbar.Combo.Tooltip", "Apply a resolution preset"));
	combo->addItem(localeText("Toolbar.Placeholder", "Resolu\u00E7\u00E3o\u2026"));
	for (const Preset &preset : kPresets)
		combo->addItem(localeText(preset.localeKey, preset.fallback));

	QPushButton *toggleButton = new QPushButton(widget);
	toggleButton->setText(QStringLiteral("\u21C4")); /* ⇄ */
	toggleButton->setToolTip(
		localeText("Toolbar.Toggle.Tooltip",
			   "Switch horizontal <-> vertical (keeps current quality)"));

	QPushButton *moreButton = new QPushButton(widget);
	moreButton->setText(QStringLiteral("?")); /* ajuda: layout e Sobre */
	moreButton->setFlat(true);
	moreButton->setFixedWidth(28);
	moreButton->setToolTip(localeText("Toolbar.Help.Tooltip",
					  "Toolbar layout and about"));

	widget->box()->addWidget(label);
	widget->box()->addWidget(combo, 1);
	widget->box()->addWidget(toggleButton);
	widget->box()->addWidget(moreButton);

	/* Atualiza o visor e o combobox com a resolução atual do OBS. */
	auto refresh = [label, combo]() {
		struct obs_video_info ovi;
		if (!obs_get_video_info(&ovi))
			return;

		label->setText(QString(QStringLiteral("%1\u00D7%2"))
				       .arg(ovi.output_width)
				       .arg(ovi.output_height));
		label->setToolTip(
			QString(QStringLiteral("Canvas %1\u00D7%2 \u00B7 Sa\u00EDda %3\u00D7%4"))
				.arg(ovi.base_width)
				.arg(ovi.base_height)
				.arg(ovi.output_width)
				.arg(ovi.output_height));

		int match = -1;
		for (const Preset &preset : kPresets) {
			if (preset.outputWidth == ovi.output_width &&
			    preset.outputHeight == ovi.output_height) {
				match = static_cast<int>(&preset - &kPresets[0]) + 1;
				break;
			}
		}

		combo->blockSignals(true);
		combo->setCurrentIndex(match);
		combo->blockSignals(false);
	};

	QObject::connect(combo, &QComboBox::currentIndexChanged, widget,
			 [widget, refresh](int index) {
				 if (index <= 0)
					 return; /* item 0 é o placeholder */
				 const Preset &preset = kPresets[index - 1];
				 applyVideo(widget, refresh, preset.baseWidth, preset.baseHeight,
					    preset.outputWidth, preset.outputHeight);
			 });

	QObject::connect(toggleButton, &QPushButton::clicked, widget,
			 [widget, refresh]() {
				 struct obs_video_info ovi;
				 if (!obs_get_video_info(&ovi))
					 return;
				 const struct obs_video_info swapped = swappedVideo(ovi);
				 applyVideo(widget, refresh, swapped.base_width,
					    swapped.base_height, swapped.output_width,
					    swapped.output_height);
			 });

	QObject::connect(moreButton, &QPushButton::clicked, widget,
			 [widget]() { widget->showLayoutMenu(QCursor::pos()); });

	/* Mantém o visor sincronizado se a resolução mudar por fora (levíssimo:
	 * só um refresh a cada 2s, sem diálogo nem trabalho pesado). */
	QTimer *syncTimer = new QTimer(widget);
	syncTimer->setInterval(2000);
	QObject::connect(syncTimer, &QTimer::timeout, widget,
			 [widget, refresh]() {
				 if (widget->isVisible())
					 refresh();
			 });
	syncTimer->start();

	refresh();

	const QString title = localeText("Toolbar.Title", "easy-aspect-ratio");
	const QByteArray titleBytes = title.toUtf8();
	if (!obs_frontend_add_dock_by_id("easy-aspect-ratio-changer", titleBytes.constData(),
					 widget))
		obs_log(LOG_WARNING, "could not register resolution toolbar dock");
	else
		obs_log(LOG_INFO, "resolution toolbar dock registered");
}
