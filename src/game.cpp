/***************************************************************************
 *   Copyright 2008-2010 Stefan Majewsky <majewsky@gmx.net>
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

#include "game.h"
#include "board.h"
#include "diamond.h"
#include "settings.h"

#include <cmath>
#include <QPainter>
#include <QTimerEvent>
#include <KGamePopupItem>
#include <KGameRenderer>
#include <KgTheme>
#include <KgThemeProvider>
#include <KGlobal>
#include <KNotification>


#include <iostream>
using namespace std;

//BEGIN global KGameRenderer instance

namespace KDiamond
{
	class ThemeProvider : public KgThemeProvider
	{
		public:
			ThemeProvider(QObject* parent = 0)
				: KgThemeProvider("Theme", parent)
			{
				discoverThemes("appdata", QLatin1String("themes"));
			}
	};

	class Renderer : public KGameRenderer
	{
		public:
			Renderer() : KGameRenderer(new ThemeProvider, 10)
			{
				setFrameSuffix(QString::fromLatin1("-%1"));
			}
	};
}

K_GLOBAL_STATIC(KDiamond::Renderer, g_renderer)

KGameRenderer* KDiamond::renderer()
{
	return g_renderer;
}

//END global KGameRenderer instance

const int UpdateInterval = 40;

Game::Game(KDiamond::GameState* state)
	: m_timerId(-1)
	, m_board(new KDiamond::Board(g_renderer))
	, m_gameState(state)
	, m_messenger(new KGamePopupItem)
{
	connect(m_board, SIGNAL(animationsFinished()), SLOT(animationFinished()));
	connect(m_board, SIGNAL(clicked(QPoint)), SLOT(clickDiamond(QPoint)));
	connect(m_board, SIGNAL(dragged(QPoint,QPoint)), SLOT(dragDiamond(QPoint,QPoint)));
	//init scene (with some default scene size that makes board coordinates equal scene coordinates)
	const int minSize = m_board->gridSize();
	setSceneRect(0.0, 0.0, minSize, minSize);
	connect(this, SIGNAL(sceneRectChanged(QRectF)), SLOT(updateGraphics()));
	connect(g_renderer->themeProvider(), SIGNAL(currentThemeChanged(const KgTheme*)), SLOT(updateGraphics()));
	addItem(m_board);
	//init messenger
	m_messenger->setMessageOpacity(0.8);
	m_messenger->setHideOnMouseClick(false);
	addItem(m_messenger);
	//init time management
	m_timerId = startTimer(UpdateInterval);
	//schedule late initialisation
	m_jobQueue << KDiamond::UpdateAvailableMovesJob;

//	//INIZIALIZZO IL RANDOM PLAYER
//	m_player = new Player(this);
}


//Checks amount of possible moves remaining
void Game::getMoves(){
	m_availableMoves.clear();
	const int gridSize = m_board->gridSize();
	for (QPoint point; point.x() < gridSize - 1; ++point.rx()){
		for (point.ry() = 0; point.y() < gridSize - 1; ++point.ry()){
			if(!m_board->hasDiamond(point)) continue;
             //guardo solo a sinistra e in basso, gli altri casi sono considerati
             //dagli altri punti
            QVector<QPoint> destinations;
            destinations.append(point + QPoint(1, 0));
            destinations.append(point + QPoint(0, 1));
            for(auto dest : destinations){
                if(m_board->hasDiamond(dest)){
                    m_board->swapDiamonds(point, dest, false); //senza animazione
                    auto figure1 = findFigure(point).points();
                    auto figure2 = findFigure(dest).points();
                    //se la mossa ha successo
                    if(!(figure1.isEmpty() && figure2.isEmpty())){
                        Move mov(point, dest);
                        mov.m_toDelete = figure1 + figure2;
                        m_availableMoves.append(mov);
                    }
                    m_board->swapDiamonds(point, dest, false); //senza animazione
                }
            }
        }
    }

    emit numberMoves(m_availableMoves.size());
    if (m_availableMoves.isEmpty()){
		m_board->clearSelection();
		m_gameState->setState(KDiamond::Finished); //TODO, forse va agginto un EndGameJob
	}
}

void Game::updateGraphics()
{
    cout << "updateGraphics" << endl;
	//calculate new metrics
	const QSize sceneSize = sceneRect().size().toSize();
	const int gridSize = m_board->gridSize();
	const int diamondSize = (int) floor(qMin(
		sceneSize.width() / (gridSize + 1.0), //the "+1" and "+0.5" make sure that some space is left on the window for the board border
		sceneSize.height() / (gridSize + 0.5)
	));
	const int boardSize = gridSize * diamondSize;
	const int leftOffset = (sceneSize.width() - boardSize) / 2.0;
	QTransform t;
	t.translate(leftOffset, 0).scale(diamondSize, diamondSize);
	m_board->setTransform(t);
	//render background
	QPixmap pix = g_renderer->spritePixmap("kdiamond-background", sceneSize);
	const KgTheme* theme = g_renderer->theme();
	const bool hasBorder = theme->customData("HasBorder").toInt() > 0;
	if (hasBorder)
	{
		const qreal borderPercentage = theme->customData("BorderPercentage").toFloat();
		const int padding = borderPercentage * boardSize;
		const int boardBorderSize = 2 * padding + boardSize;
		const QPixmap boardPix = g_renderer->spritePixmap("kdiamond-border", QSize(boardBorderSize, boardBorderSize));
		QPainter painter(&pix);
		painter.drawPixmap(QPoint(leftOffset - padding, -padding), boardPix);
	}
	setBackgroundBrush(pix);
}

void Game::clickDiamond(const QPoint& point)
{
	if (m_gameState->state() != KDiamond::Playing)
		return;
	//do not allow more than two selections
	const bool isSelected = m_board->hasSelection(point);
	if (!isSelected && m_board->selections().count() == 2)
		return;
	//select only adjacent diamonds (i.e. if a distant diamond is selected, deselect the first one)
	foreach (const QPoint& point2, m_board->selections())
	{
		const int diff = qAbs(point2.x() - point.x()) + qAbs(point2.y() - point.y());
		if (diff > 1)
			m_board->setSelection(point2, false);
	}
	//toggle selection state
	m_board->setSelection(point, !isSelected);
	if (m_board->selections().count() == 2){
		m_jobQueue << KDiamond::SwapDiamondsJob;
    }
}

void Game::dragDiamond(const QPoint& point, const QPoint& direction)
{
	//direction must not be null, and must point along one axis
	if ((direction.x() == 0) ^ (direction.y() == 0))
	{
		//find target indices
		const QPoint point2 = point + direction;
		if (!m_board->hasDiamond(point2))
			return;
		//simulate the clicks involved in this operation
		m_board->clearSelection();
		m_board->setSelection(point, true);
		m_board->setSelection(point2, true);
		m_jobQueue << KDiamond::SwapDiamondsJob;
	}
}

void Game::sleep(int ms) const{
#ifdef Q_OS_WIN
    Sleep(uint(ms));
#else
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
    nanosleep(&ts, NULL);
#endif
}

void Game::timerEvent(QTimerEvent* event)
{
    //cout << "timerEvent" << endl;

	//propagate event to superclass if necessary
	if (event->timerId() != m_timerId)
	{
		QGraphicsScene::timerEvent(event);
		return;
	}
	//do not handle any jobs while animations are running
	if (m_gameState->state() == KDiamond::Paused || m_board->hasRunningAnimations())
	{
		killTimer(m_timerId);
		m_timerId = -1;
		return;
	}

	//anything to do in this update?
	if (m_jobQueue.isEmpty())
	{ /***IMPLEMENTO QUI IL RANDOM PLAYER*****/
        if(m_availableMoves.size() > 0){
            m_board->clearSelection();
            auto m = m_availableMoves[qrand() % m_availableMoves.size()];
            clickDiamond(m.from());
            clickDiamond(m.to());
            cout << "MOVE :  " << m.from().x() << "  " << m.from().y() << " --> " << m.to().x() << "  " << m.to().y() <<  endl;
//                cout << "*****MOVES********" << endl;
//            for(auto M : m_availMoves){
//                cout << "M :  " << M.first.x() << "  " << M.first.y() << " --> " << M.second.x() << "  " << M.second.y() <<  endl;
//            }
        }
        return;
	}
	//execute next job in queue
	const KDiamond::Job job = m_jobQueue.takeFirst();
	switch (job)
	{
		case KDiamond::SwapDiamondsJob: {
			if (m_board->selections().count() != 2)
				break; //this can be the case if, during a cascade, two diamonds are selected (inserts SwapDiamondsJob) and then deselected
			//ensure that the selected diamonds are neighbors (this is not necessarily the case as diamonds can move to fill gaps)
			const QList<QPoint> points = m_board->selections();
			cout << "SWAPPING" << endl;
            sleep(1000); //Sennò non si vede un cazzo
			m_board->clearSelection();
			const int dx = qAbs(points[0].x() - points[1].x());
			const int dy = qAbs(points[0].y() - points[1].y());
			if (dx + dy != 1)
				break;
			//start a new cascade
			m_gameState->resetCascadeCounter();
			//copy selection info into another storage (to allow the user to select the next two diamonds while the cascade runs)
			m_swappingDiamonds = points;
			m_jobQueue << KDiamond::RemoveFiguresJob; //We already insert this here to avoid another conditional statement.
		} //fall through
		case KDiamond::RevokeSwapDiamondsJob:
			//invoke movement
			KNotification::event("move");
			m_board->swapDiamonds(m_swappingDiamonds[0], m_swappingDiamonds[1]);
			break;

		case KDiamond::RemoveFiguresJob: {
            cout<<"Job::RemoveRowJob" << endl;
			const QVector<Figure> figuresToRemove = findFigures();
            cout<<"Num figures to remove =" << figuresToRemove.size() << endl;
			if (figuresToRemove.isEmpty()){
				//no diamond rows were formed by the last move -> revoke movement (unless we are in a cascade)
				if (!m_swappingDiamonds.isEmpty()){
					m_jobQueue.prepend(KDiamond::RevokeSwapDiamondsJob);
				}
				else {
                    m_jobQueue << KDiamond::UpdateAvailableMovesJob;
                }
			}
			else{ //C'è qualcosa da rimuovere

				//all moves may now be out-dated - flush the moves list
				if (!m_availableMoves.isEmpty()){
					m_availableMoves.clear();
                    emit numberMoves(-1);
				}
				//Controllo se sto swappando e dato che lo swap ha avuto successo
                //salvo i punti swappati perché mi serviranno
                auto puntiSwappati = m_swappingDiamonds;
                // incremento il numero di mosse ed elimino la selezione
				if(!m_swappingDiamonds.isEmpty()){
//                    m_gameState->updateMovesLeft(); TODO
                    m_swappingDiamonds.clear();
                    m_board->clearSelection();
				}


				//invoke remove animation, then fill gaps immediately after the animation
				KNotification::event("remove");

				              //Segno i punti ed elimino le figure
                cout<<"### Removing Figures" << endl;
//                printBoard();
                for(const auto& fig : figuresToRemove){
                    //invoke remove animation, then fill gaps immediately after the animation
                    for(const QPoint& diamondPos: fig.points()){
                        if(m_board->hasDiamond(diamondPos)){ //potrebbe essere (casi rari) che era già stato scoppiato
                            if(m_board->diamond(diamondPos)->isJolly()){
                                removeJolly(diamondPos);
                            }
                            else {
                                removeDiamond(diamondPos);
                            }
                        }
                    }
                }

				m_jobQueue.prepend(KDiamond::FillGapsJob);
			}
			break;
		}

		case KDiamond::FillGapsJob:
			//fill gaps
			m_board->fillGaps();
			m_jobQueue.prepend(KDiamond::RemoveFiguresJob); //allow cascades (i.e. clear rows that have been formed by falling diamonds)
			break;
		case KDiamond::UpdateAvailableMovesJob:
			if (m_gameState->state() != KDiamond::Finished)
				getMoves();
			break;
		case KDiamond::EndGameJob:
			emit pendingAnimationsFinished();
			killTimer(m_timerId);
			m_timerId = -1;
			break;
	}
}

void Game::removeDiamond(const QPoint& point){
    cout << "SCOPPIO Diamante"  <<" in " << point.x() << " " << point.y() << endl;
    m_gameState->addPoints(1);
    m_board->removeDiamond(point);
    cout << "SCOPPIATO" << endl;
}


//TODO Inserire busta e cookie
void Game::removeJolly(const QPoint& point){
    cout << "SCOPPIO Jolly"  <<" in " << point.x() << " " << point.y() << endl;
    auto jtype = m_board->diamond(point)->jollyType();
    removeDiamond(point);


    //TODO Che succede se incontro un Jolly? Lo esplodo come jolly?
    if(jtype == JollyType::H){
        int y = point.y();
        for(int x = 0; x < m_board->gridSize(); ++x) {
            removeDiamond({x,y});
        }
    }

   if(jtype == JollyType::V){
        int x = point.y();
        for(int y = 0; y < m_board->gridSize(); ++y) {
            removeDiamond({x,y});
        }
    }
}

void Game::showHint()
{
	if (m_availableMoves.isEmpty() || !m_board->selections().isEmpty())
		return;
	auto m = m_availableMoves.value(qrand() % m_availableMoves.size());
	m_board->setSelection(m.from(), true);
    m_board->setSelection(m.to(), true);
	m_gameState->removePoints(3);
}

void Game::animationFinished()
{
	if (m_timerId == -1)
		m_timerId = startTimer(UpdateInterval);
}

void Game::stateChange(KDiamond::State state)
{
	m_board->setPaused(state == KDiamond::Paused);
	switch ((int) state)
	{
		case KDiamond::Finished:
			m_board->clearSelection();
			m_jobQueue << KDiamond::EndGameJob;
			break;
		case KDiamond::Playing:
			if (m_timerId == -1)
				m_timerId = startTimer(UpdateInterval);
			break;
	}
}

void Game::message(const QString &message)
{
	if (message.isEmpty())
		m_messenger->forceHide();
	else
		m_messenger->showMessage(message, KGamePopupItem::TopLeft);
}

const QVector<Move>& Game::availMoves() const{
    return m_availableMoves;
}


//ritorna la riga verticale contenente il punto (escluso il punto stesso)
QVector<QPoint> Game::findRowV(const QPoint& point){
    QVector<QPoint> row;
    #define C(X, Y) (m_board->hasDiamond(QPoint(X, Y)) ? m_board->diamond(QPoint(X, Y))->color() : KDiamond::Selection)
    auto currColor = m_board->diamond(point)->color();  //ATTENZIONE Non controllo che il colore sia valido (!=Selection)
    int yt = point.y() + 1;
    const int x = point.x();
    while(C(x, yt) == currColor){ //ciclo verso il basso
        row.append(QPoint(x, yt));
        yt++;
    }

    yt = point.y() - 1;
    while(C(x, yt) == currColor){ //ciclo verso l'alto
        row.append(QPoint(x, yt));
        yt--;
    }
    #undef C
    return row;
}

//ritorna la riga orizzontale contenente il punto (escluso il punto stesso)
QVector<QPoint> Game::findRowH(const QPoint& point){
    QVector<QPoint> row;
    #define C(X, Y) (m_board->hasDiamond(QPoint(X, Y)) ? m_board->diamond(QPoint(X, Y))->color() : KDiamond::Selection)
    auto currColor = m_board->diamond(point)->color();  //ATTENZIONE Non controllo che il colore sia valido (!=Selection)
    int xt = point.x() + 1;
    const int y = point.y();
    while(C(xt, y) == currColor){ //ciclo verso destra
        row.append(QPoint(xt, y));
        xt++;
    }

    xt = point.x() - 1;
    while(C(xt, y) == currColor){ //ciclo verso sinistra
        row.append(QPoint(xt, y));
        xt--;
    }
    #undef C
    return row;
}


QVector<Figure> Game::findFigures(){
	QVector<Figure> diamonds;
	const int gridSize = m_board->gridSize();
	QVector<bool> inFigure(gridSize * gridSize, false);
	for (QPoint point; point.x() < gridSize; ++point.rx()){
		for (point.ry() = 0; point.y() < gridSize ; ++point.ry()){
            //controllo che ci sia un diamante e che non sia già parte di una figura
            if(m_board->hasDiamond(point) && !inFigure[point.x() + gridSize * point.y()]){
                auto figure = findFigure(point);
                for(auto& p : figure.points()){
                    inFigure[p.x() + gridSize * p.y()] = true;
                }
                if (figure.size() > 0){
                    diamonds += figure;
                }
            }
        }
	}

	return diamonds;
}

Figure Game::findFigure(QPoint point){
    auto rH = findRowH(point);
    auto rV = findRowV(point);

    FigureType type;
    QVector<QPoint> points;
    //Controllo che figura ho trovato. Si può rendere pìù efficiente
    if(rH.size() >= 2 && rV.size() < 2){
    //riga orizzontale
        points.append(point);
        points += rH;
        type = FigureType::RowH;
        //controllo se si crea una T o una L
        for(auto p : rH){
            auto rV2 = findRowV(p);
            if(rV2.size() >= 2){
                points += rV2;
                type = FigureType::LT;
                //ATTENZIONE si possono avere più file verticali o posso
                //mettere un break?
                //direi che non si possono avere più righe verticali: al massimo
                //se ne forma una
                break;
            }
        }
    } else if(rH.size() < 2 && rV.size() >= 2){
    // riga verticale
        points.append(point);
        points += rV;
        type = FigureType::RowV;
        //controllo se si crea una T o una L
        for(auto p : rV){
            auto rH2 = findRowH(p);
            if(rH2.size() >= 2){
                points += rH2;
                type = FigureType::LT;
                //ATTENZIONE si possono avere più file verticali o posso
                //mettere un break?
                // direi che non si possono creare
                break;
            }
        }

    } else if(rH.size() >= 2 && rV.size() >= 2){
        //ho trovato una T o una L
        points.append(point);
        points += rH;
        points += rV;
        type = FigureType::LT;
        //ATTENZIONE non controllo la creazione di figure più complesse
        // che si potrebbero formare quando calano i diamanti dall'alto

        //qua se rH.size()=2 e rV.size()=2 allora devi creare una busta in point
        //se però, per esempio, rH.size()=3 e rV.size()=2, c'è un conflitto:
        //-creo un jolly verticale in point?
        //-creo sempre una busta in point?
    }

    return Figure(points, type);
}



#include "game.moc"
