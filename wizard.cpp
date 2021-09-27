/* Copyright (c) 2021, sarutora
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "wizard.h"
#include <QVBoxLayout>
#include <QProcess>
#include <QDir>
#include <QMessageBox>
#include <QRegularExpression>

Wizard::Wizard(QWidget *parent):
    QWizard(parent)
{
    setPage(Page_Intro, new IntroPage(this, &projectPath));
    setPage(Page_Info, new InfoPage);
    setPage(Page_File, new FilePage);
    setPage(Page_Conclusion, new ConclusionPage);
    setStartId(Page_Intro);
    setWindowTitle(tr("New Project or File"));
}

void Wizard::accept()
{
    if (hasVisitedPage(Page_Info)) {
        QString projectName = field("projectName").toString();        QStringList args;
        QString crateType;
        QString directory = field("directory").toString();
        openName = directory + "/";
        if (field("bin").toBool()) {
            crateType = "--bin";
            openName += "main.rs";
        } else {
            crateType = "--lib";
            openName += "lib.rs";
        }
        args << "new" << projectName << crateType;
        if (!field("vcs").toBool()) {
            args << "--vcs" << "none";
        }

        QDir::setCurrent(directory);        QProcess *cargo = new QProcess(this);
        cargo->start("cargo", args);
        cargo->waitForFinished();
        int status = cargo->exitCode();
        if (status != 0) {
            openName.clear();
            QString msg = cargo->readAllStandardError();
            QRegularExpression err("error[^\n]+");
            QRegularExpressionMatch match = err.match(msg);
            if (match.hasMatch())
                QMessageBox::warning(this, "Error", match.captured());
            else
                throw "Error with Cargo: " + msg;
        }
    } else {
        QString fileName = field("fileName").toString();
        QString directory = field("fileDirectory").toString();
        directory += '/' + fileName;
        QFile file(directory);
        if (!file.exists()) {
            if (file.open(QIODevice::ReadWrite))
                openName = directory;
            else
                QMessageBox::warning(this, "Error", "Could not create " + directory);
        } else
            QMessageBox::warning(this, "Error", directory + " already exists.");
    }

    QDialog::accept();
}

QWizardPage *createIntroPage()
{
    QWizardPage *page = new QWizardPage;
    page->setTitle("Introduction");

    QLabel *label = new QLabel("This wizard will help you register your copy "
                               "of Super Product Two.");
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    page->setLayout(layout);

    return page;
}

IntroPage::IntroPage(QWidget *parent, QString *projectPath)
    : QWizardPage(parent), projectPath(projectPath)
{
    setTitle(tr("Introduction"));

    topLabel = new QLabel(tr("This wizard will help you create a file or project."));
    topLabel->setWordWrap(true);

    fileRadioButton = new QRadioButton(tr("Create a new &file"));
    projectRadioButton = new QRadioButton(tr("Create a new &project"));
    fileRadioButton->setChecked(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(topLabel);
    layout->addWidget(fileRadioButton);
    layout->addWidget(projectRadioButton);
    setLayout(layout);
}

void IntroPage::initializePage()
{
    if (projectPath->isEmpty()) {
        fileRadioButton->setVisible(false);
        projectRadioButton->setChecked(true);
    }
}

int IntroPage::nextId() const
{
    if (fileRadioButton->isChecked()) {
        return Wizard::Page_File;
    } else {
        return Wizard::Page_Info;
    }
}

FilePage::FilePage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("File Information"));
    setSubTitle(tr("Specify basic information about the file you want to create."));

    fileNameLabel = new QLabel(tr("&Name:"));
    fileNameLineEdit = new QLineEdit;
    fileNameLabel->setBuddy(fileNameLineEdit);
    directoryLabel = new QLabel(tr("&Path:"));
    directoryLineEdit = new QLineEdit;
    directoryLabel->setBuddy(directoryLineEdit);


    registerField("fileName*", fileNameLineEdit);
    registerField("fileDirectory*", directoryLineEdit);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(fileNameLabel, 0, 0);
    layout->addWidget(fileNameLineEdit, 0, 1);
    layout->addWidget(directoryLabel, 1, 0);
    layout->addWidget(directoryLineEdit, 1, 1);
    setLayout(layout);
}

int FilePage::nextId() const
{
     return Wizard::Page_Conclusion;
}

InfoPage::InfoPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Project Information"));
    setSubTitle(tr("Specify basic information about the project you want to create."));

    projectNameLabel = new QLabel(tr("&Name:"));
    projectNameLineEdit = new QLineEdit;
    projectNameLabel->setBuddy(projectNameLineEdit);
    directoryLabel = new QLabel(tr("&Directory:"));
    directoryLineEdit = new QLineEdit;
    directoryLabel->setBuddy(directoryLineEdit);

    vcsCheckBox = new QCheckBox(tr("&Initialize a new git repository"));    vcsCheckBox->setChecked(true);

    groupBox = new QGroupBox(tr("Type"));
    binRadioButton = new QRadioButton(tr("&Binary"));
    libRadioButton = new QRadioButton(tr("&Library"));
    binRadioButton->setChecked(true);


    registerField("projectName*", projectNameLineEdit);
    registerField("directory*", directoryLineEdit);
    registerField("vcs", vcsCheckBox);
    registerField("bin", binRadioButton);
    registerField("lib", libRadioButton);

    QVBoxLayout *groupBoxLayout = new QVBoxLayout;
    groupBoxLayout->addWidget(binRadioButton);
    groupBoxLayout->addWidget(libRadioButton);
    groupBox->setLayout(groupBoxLayout);


    QGridLayout *layout = new QGridLayout;
    layout->addWidget(projectNameLabel, 0, 0);
    layout->addWidget(projectNameLineEdit, 0, 1);
    layout->addWidget(directoryLabel, 1, 0);
    layout->addWidget(directoryLineEdit, 1, 1);
    layout->addWidget(vcsCheckBox, 2, 0, 1, 2);
    layout->addWidget(groupBox, 3, 0, 1, 2);
    setLayout(layout);
}

void InfoPage::initializePage()
{
    directoryLineEdit->setText(QDir::homePath());
}

int InfoPage::nextId() const
{
    return Wizard::Page_Conclusion;
}

ConclusionPage::ConclusionPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Conclusion"));

    label = new QLabel;
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);
}

void ConclusionPage::initializePage()
{
    QString finishText = wizard()->buttonText(QWizard::FinishButton);
    finishText.remove('&');
        label->setText(tr("Click %1 to finish.")
                       .arg(finishText));
}
