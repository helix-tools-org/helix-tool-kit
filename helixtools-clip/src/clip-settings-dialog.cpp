/*
 * clip-settings-dialog.cpp - Tools menu settings dialog for helixtools-clip
 *
 * Copyright (C) 2026 HelixTools
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "clip-settings-dialog.h"
#include "helixtools-clip.h"

#include <obs-frontend-api.h>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QGroupBox>

static void on_clip_settings(void *)
{
    QWidget *parent = static_cast<QWidget *>(obs_frontend_get_main_window());
    HelixToolsClip *clip = HelixToolsClip::instance();
    if (!clip)
        return;

    QDialog dialog(parent);
    dialog.setWindowTitle("HelixTools Clip Settings");
    dialog.setMinimumWidth(420);

    auto *layout = new QVBoxLayout(&dialog);

    auto *group = new QGroupBox("Clip notifications", &dialog);
    auto *groupLayout = new QVBoxLayout(group);

    auto *toastCheck = new QCheckBox("Show toast popup", group);
    toastCheck->setChecked(clip->getShowToast());
    groupLayout->addWidget(toastCheck);

    auto *overlayCheck = new QCheckBox("Show in-stream overlay alert", group);
    overlayCheck->setChecked(clip->getShowOverlay());
    groupLayout->addWidget(overlayCheck);

    auto *soundCheck = new QCheckBox("Play a sound", group);
    soundCheck->setChecked(clip->getPlaySound());
    groupLayout->addWidget(soundCheck);

    auto *soundRow = new QHBoxLayout();
    auto *soundPathEdit = new QLineEdit(clip->getCustomSound(), group);
    soundPathEdit->setPlaceholderText("Leave blank for the default system sound");
    auto *browseButton = new QPushButton("Browse...", group);
    soundRow->addWidget(soundPathEdit);
    soundRow->addWidget(browseButton);
    groupLayout->addLayout(soundRow);

    QObject::connect(browseButton, &QPushButton::clicked, group, [&]() {
        QString path = QFileDialog::getOpenFileName(
            &dialog, "Choose Clip Notification Sound",
            soundPathEdit->text(),
            "Audio Files (*.wav *.mp3 *.wma);;All Files (*.*)");
        if (!path.isEmpty())
            soundPathEdit->setText(path);
    });

    layout->addWidget(group);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    clip->SetShowToast(toastCheck->isChecked());
    clip->SetShowOverlay(overlayCheck->isChecked());
    clip->SetPlaySound(soundCheck->isChecked());
    clip->SetCustomSound(soundPathEdit->text());
}

void helixtools_clip_init_menu(void)
{
    obs_frontend_add_tools_menu_item("HelixTools Clip Settings...",
                                     on_clip_settings, nullptr);
}
