/*
Toolbar de Resolução
Copyright (C) 2026 Seu Nome <seu-email@exemplo.com>

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

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

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

/* Avisa que a resolução não pode mudar com saída ativa: o próprio OBS
 * bloqueia (obs_reset_video devolve OBS_VIDEO_CURRENTLY_ACTIVE). */
void warnVideoActive(QWidget *parent)
{
	QString title = localeText("Toolbar.Active.Title", "Output is active");
	QString text = localeText("Toolbar.Active.Text",
				  "Recording or streaming is active. OBS cannot change the "
				  "resolution while an output is running. Stop the output "
				  "first, then try again.");
	QMessageBox::warning(parent, title, text);
}

void warnApplyFailed(QWidget *parent, int result)
{
	QString title = localeText("Toolbar.Error.Title", "Could not change resolution");

	QString detail;
	switch (result) {
	case OBS_VIDEO_INVALID_PARAM:
		detail = localeText("Toolbar.Error.InvalidParam",
				    "The resolution values are invalid.");
		break;
	case OBS_VIDEO_CURRENTLY_ACTIVE:
		detail = localeText("Toolbar.Error.Active",
				    "Video is currently active (stop recording/streaming first).");
		break;
	case OBS_VIDEO_NOT_SUPPORTED:
		detail = localeText("Toolbar.Error.NotSupported",
				    "The video adapter does not support these settings.");
		break;
	case OBS_VIDEO_MODULE_NOT_FOUND:
		detail = localeText("Toolbar.Error.ModuleNotFound",
				    "The graphics module was not found.");
		break;
	default:
		detail = localeText("Toolbar.Error.Generic", "OBS returned a generic failure.");
		break;
	}

	QString text = localeText("Toolbar.Error.Text", "OBS refused the resolution change:") + " " +
		       detail;
	QMessageBox::warning(parent, title, text);
}

/* Troca horizontal <-> vertical, mantendo a "qualidade" (a menor dimensão).
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

} // namespace

void quickres_create_dock()
{
	QWidget *widget = new QWidget();
	QHBoxLayout *layout = new QHBoxLayout(widget);
	layout->setContentsMargins(6, 3, 6, 3);
	layout->setSpacing(6);

	QLabel *label = new QLabel(QStringLiteral("\xE2\x80\x94"), widget);
	// texto inicial: um traço até o primeiro refresh
	label->setMinimumWidth(84);
	label->setAlignment(Qt::AlignCenter);

	QComboBox *combo = new QComboBox(widget);
	combo->setToolTip(localeText("Toolbar.Combo.Tooltip", "Apply a resolution preset"));
	combo->addItem(localeText("Toolbar.Placeholder", "Resolu\xC3\xA7\xC3\xA3\xE2\x80\xA6"));
	for (const Preset &preset : kPresets)
		combo->addItem(localeText(preset.localeKey, preset.fallback));

	QPushButton *toggleButton = new QPushButton(widget);
	toggleButton->setText(QStringLiteral("\xE2\x87\x84")); /* ⇄ */
	toggleButton->setToolTip(
		localeText("Toolbar.Toggle.Tooltip",
			   "Switch horizontal <-> vertical (keeps current quality)"));

	layout->addWidget(label);
	layout->addWidget(combo, 1);
	layout->addWidget(toggleButton);

	/* Atualiza o visor e o combobox com a resolução atual do OBS. */
	auto refresh = [label, combo]() {
		struct obs_video_info ovi;
		if (!obs_get_video_info(&ovi))
			return;

		label->setText(QString(QStringLiteral("%1\xC3\x97%2"))
				       .arg(ovi.output_width)
				       .arg(ovi.output_height));
		label->setToolTip(
			QString(QStringLiteral("Canvas %1\xC3\x97%2 \xC2\xB7 Sa\xC3\xADda %3\xC3\x97%4"))
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

	auto applyVideo = [widget, refresh](struct obs_video_info ovi) {
		/* Igual ao dialogo de video do OBS: bloqueia quando ha saida com
		 * encoder rodando e aplica via obs_reset_video (Canvas + Saida). */
		if (obs_video_active()) {
			warnVideoActive(widget);
			return;
		}

		const int result = obs_reset_video(&ovi);
		if (result != OBS_VIDEO_SUCCESS) {
			warnApplyFailed(widget, result);
			return;
		}

		refresh();
		/* garante visor correto caso o OBS recalcule depois */
		QTimer::singleShot(600, widget, [refresh]() { refresh(); });
	};

	QObject::connect(combo, &QComboBox::currentIndexChanged, widget,
			 [widget, applyVideo](int index) {
				 if (index <= 0)
					 return; /* item 0 é o placeholder */
				 struct obs_video_info ovi;
				 if (!obs_get_video_info(&ovi))
					 return;
				 const Preset &preset = kPresets[index - 1];
				 ovi.base_width = preset.baseWidth;
				 ovi.base_height = preset.baseHeight;
				 ovi.output_width = preset.outputWidth;
				 ovi.output_height = preset.outputHeight;
				 applyVideo(ovi);
			 });

	QObject::connect(toggleButton, &QPushButton::clicked, widget,
			 [widget, applyVideo]() {
				 struct obs_video_info ovi;
				 if (!obs_get_video_info(&ovi))
					 return;
				 applyVideo(swappedVideo(ovi));
			 });

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

	const QString title = localeText("Toolbar.Title", "Resolu\xC3\xA7\xC3\xA3o");
	const QByteArray titleBytes = title.toUtf8();
	if (!obs_frontend_add_dock_by_id("quick-res-toolbar", titleBytes.constData(), widget))
		obs_log(LOG_WARNING, "could not register resolution toolbar dock");
	else
		obs_log(LOG_INFO, "resolution toolbar dock registered");
}
