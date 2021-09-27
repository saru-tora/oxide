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

#ifndef WIZARD_H
#define WIZARD_H

#include <QWizard>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QRadioButton>

class Wizard: public QWizard
{
    Q_OBJECT
public:
    enum { Page_Intro, Page_Info, Page_File, Page_Conclusion };
    Wizard(QWidget *parent = nullptr);
    void accept() override;
    QString openName;
    QString projectPath;
};

QWizardPage *createIntroPage();

class IntroPage : public QWizardPage
{
    Q_OBJECT

public:
    IntroPage(QWidget *parent = nullptr, QString *projectPath = nullptr);

    int nextId() const override;

protected:
    void initializePage() override;

private:
    QLabel *topLabel;
    QRadioButton *fileRadioButton;
    QRadioButton *projectRadioButton;
    QString *projectPath;
};

class FilePage : public QWizardPage
{
    Q_OBJECT

public:
    FilePage(QWidget *parent = nullptr);

    int nextId() const override;

private:
    QLabel *fileNameLabel;
    QLabel *directoryLabel;
    QLineEdit *fileNameLineEdit;
    QLineEdit *directoryLineEdit;
};

class InfoPage : public QWizardPage
{
    Q_OBJECT

public:
    InfoPage(QWidget *parent = nullptr);
    int nextId() const override;

protected:
    void initializePage() override;

private:
    QLabel *projectNameLabel;
    QLabel *directoryLabel;
    QLineEdit *projectNameLineEdit;
    QLineEdit *directoryLineEdit;
    QCheckBox *vcsCheckBox;
    QGroupBox *groupBox;
    QRadioButton *binRadioButton;
    QRadioButton *libRadioButton;
};

class ConclusionPage : public QWizardPage
{
    Q_OBJECT

public:
    ConclusionPage(QWidget *parent = nullptr);

protected:
    void initializePage() override;

private:
    QLabel *label;
};

#endif // WIZARD_H
