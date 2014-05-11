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

#ifndef KDIAMOND_GAME_H
#define KDIAMOND_GAME_H

class Diamond;
#include "game-state.h"

class QAbstractAnimation;
#include <QGraphicsScene>
class KGamePopupItem;
class KGameRenderer;

#include <ctime> //per pause()
#include <utility>
using namespace std;

#ifdef Q_OS_WIN
#include <windows.h> // for Sleep
#endif




namespace KDiamond{
	//jobs to be done during the board update
    enum  Job {
            SwapDiamondsJob = 1, //swap selected diamonds
            RemoveFiguresJob, //remove complete rows of diamonds and add points
            RevokeSwapDiamondsJob, //revoke swapping of diamonds (will be requested by the RemoveRowsJob if no rows have been formed)
            FillGapsJob,
            UpdateAvailableMovesJob, //find and count available moves after the board has been changed
            EndGameJob //announce end of game
    };
	class Board;

	KGameRenderer* renderer();
}

enum class FigureType {
    None,
    RowH = 1,
    RowV,
    LT
};

class Figure{
public:
    FigureType m_type;
    QVector<QPoint> m_points;

    Figure() {}

    Figure(QVector<QPoint> points, FigureType type)
        : m_points(points)
        , m_type(type) {}

    QVector<QPoint> points() const{
        return m_points;
    }

    FigureType type() const {
        return m_type;
    }

    int size() const {
        return m_points.size();
    }
};


class Move{
    friend class Game;
public:
    Move(){}

    Move(const QPoint& from, const QPoint& to)
        : m_from(from)
        , m_to(to){}

    QPoint from() const{
        return m_from;
    }

    QPoint to() const {
        return m_to;
    }

    const QVector<QPoint>& toDelete(){
        return m_toDelete;
    }

    int numToDelete() const{
        return m_toDelete.size();
    }


private:
    QPoint m_from;
    QPoint m_to;
    QVector<QPoint> m_toDelete;
};

class Game : public QGraphicsScene
{
	Q_OBJECT
	public:
		Game(KDiamond::GameState* state);
	public Q_SLOTS:
		void updateGraphics();

		void clickDiamond(const QPoint& point);
		void dragDiamond(const QPoint& point, const QPoint& direction);

		void animationFinished();
		void message(const QString &message);
		void stateChange(KDiamond::State state);
		void showHint();
        void sleep(int ms) const;

	Q_SIGNALS:
		void boardResized();
		void numberMoves(int moves);
		void pendingAnimationsFinished();
	protected:
		virtual void timerEvent(QTimerEvent* event);
	private:
//		QList<QPoint> findCompletedRows();
        QVector<Figure> findFigures();
        Figure findFigure(QPoint point);
        QVector<QPoint> findRowH(const QPoint& point);
        QVector<QPoint> findRowV(const QPoint& point);
		void getMoves();
        const QVector<Move>& availMoves() const;
		void removeDiamond(const QPoint& point);
		void removeJolly(const QPoint& point);

	private:
		QList<KDiamond::Job> m_jobQueue;
        QVector<Move> m_availableMoves;
		QList<QPoint> m_swappingDiamonds;
		int m_timerId;
		KDiamond::Board* m_board;
		KDiamond::GameState *m_gameState;
//		Player* m_player; //PROBLEMI CON REFERENZE CIRCOLARI
		KGamePopupItem *m_messenger;
};

#endif //KDIAMOND_GAME_H
