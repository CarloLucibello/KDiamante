/***************************************************************************
 *   Copyright 2008-2009 Stefan Majewsky <majewsky@gmx.net>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 ***************************************************************************/

#include "mainwindow.h"
#include "settings.h"

#include <ctime>
#include <KApplication>
#include <KAboutData>
#include <KCmdLineArgs>
#include <KGlobal>
#include <KLocale>
#include <KStandardDirs>
#include <KgDifficulty>

#include <iostream>
using namespace std;

static const char description[] = I18N_NOOP("KDiamond, a three-in-a-row game.");
static const char version[] = "1.4";

int main(int argc, char ** argv)
{
	qsrand(time(0));
	KAboutData about("kdiamond", 0, ki18nc("The application's name", "KDiamond"), version, ki18n(description),
		KAboutData::License_GPL, ki18n("(C) 2008-2010 Stefan Majewsky and others"), KLocalizedString(), "http://games.kde.org/kdiamond" );
	about.addAuthor(ki18n("Stefan Majewsky"), ki18n("Original author and current maintainer"), "majewsky@gmx.net");
	about.addAuthor(ki18n("Paul Bunbury"), ki18n("Gameplay refinement"), "happysmileman@googlemail.com");
	about.addCredit(ki18n("Eugene Trounev"), ki18n("Default theme"), "eugene.trounev@gmail.com");
	about.addCredit(ki18n("Felix Lemke"), ki18n("Classic theme"), "lemke.felix@ages-skripte.org");
	about.addCredit(ki18n("Jeffrey Kelling"), ki18n("Technical consultant"), "kelling.jeffrey@ages-skripte.org");
	KCmdLineArgs::init(argc, argv, &about);

	KApplication app;
	KGlobal::locale()->insertCatalog( QLatin1String( "libkdegames" ));
	//resource directory for KNewStuff2 (this call causes the directory to be created; its existence is necessary for the downloader)
	KStandardDirs::locateLocal("appdata", "themes/");

	Kg::difficulty()->addStandardLevelRange(
		KgDifficultyLevel::VeryEasy,
		KgDifficultyLevel::VeryHard
	);

	// see if we are starting with session management
	if (app.isSessionRestored())
	{
		RESTORE(MainWindow);
	}
	else
	{
		MainWindow* window = new MainWindow;
		window->show();
		cout << "HERE SHOW" << endl;
	}
	return app.exec();
}
