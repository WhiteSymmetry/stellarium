#ifndef _STOREDVIEWDIALOG_P_HPP_
#define _STOREDVIEWDIALOG_P_HPP_

#include "SceneInfo.hpp"

#include <QDebug>
#include <QTextEdit>
#include <QAbstractListModel>

//! A custom QTextEdit subclass that has an editingFinished() signal like QLineEdit.
class CustomTextEdit : public QTextEdit
{
	Q_OBJECT
public:
	CustomTextEdit(QWidget* parent = NULL) : QTextEdit(parent), textChanged(false), trackChange(false)
	{
		connect(this,&QTextEdit::textChanged,this,&CustomTextEdit::handleTextChange);
	}
protected:
	void focusInEvent(QFocusEvent* e)
	{
		trackChange = true;
		textChanged = false;
		QTextEdit::focusInEvent(e);
	}

	void focusOutEvent(QFocusEvent *e)
	{
		QTextEdit::focusOutEvent(e);
		trackChange = false;

		if(textChanged)
		{
			textChanged = false;
			emit editingFinished();
		}
	}
signals:
	//! Emitted when focus lost and text was changed
	void editingFinished();
private slots:
	void handleTextChange()
	{
		if(trackChange)
			textChanged = true;
	}

private:
	bool textChanged, trackChange;
};

class StoredViewModel : public QAbstractListModel
{
	Q_OBJECT
public:
	StoredViewModel(QObject* parent = NULL) : QAbstractListModel(parent)
	{ }

	int rowCount(const QModelIndex &parent) const
	{
		if(parent.isValid())
			return 0;
		return global.size() + user.size();
	}

	QVariant data(const QModelIndex &index, int role) const
	{
		if(role == Qt::DisplayRole || role == Qt::EditRole)
		{
			return getViewAtIdx(index.row()).label;
		}
		if(role == Qt::DecorationRole)
		{
			if(getViewAtIdx(index.row()).isGlobal)
			{
				//TODO use a proper lock icon or something here
				return QIcon(":/graphicGui/folder.png");
			}
		}
		return QVariant();
	}

	const StoredView& getViewAtIdx(int idx) const
	{
		if(idx >= global.size())
		{
			return user[idx-global.size()];
		}
		else
		{
			return global[idx];
		}
	}

	StoredView& getViewAtIdx(int idx)
	{
		if(idx >= global.size())
		{
			return user[idx-global.size()];
		}
		else
		{
			return global[idx];
		}
	}


	QModelIndex addUserView(StoredView v)
	{
		int idx = global.size() + user.size();
		beginInsertRows(QModelIndex(),idx,idx);
		user.append(v);
		endInsertRows();
		persistUserViews();
		return index(idx);
	}

	void deleteView(int idx)
	{
		const StoredView& v = getViewAtIdx(idx);
		if(!v.isGlobal)
		{
			int useridx = idx - global.size();

			beginRemoveRows(QModelIndex(),idx,idx);
			user.removeAt(useridx);
			endRemoveRows();

			persistUserViews();
		}
		else
		{
			qWarning()<<"[StoredViewDialog] Cannot delete global view";
		}
	}

	void persistUserViews()
	{
		qDebug()<<"[StoredViewDialog] Persisting user views...";
		StoredView::saveUserViews(currentScene,user);
	}

	void updatedAtIdx(int idx)
	{
		QModelIndex mIdx = index(idx);
		persistUserViews();
		emit dataChanged(mIdx,mIdx);
	}

	SceneInfo getScene() { return currentScene; }

public slots:
	void setScene(const SceneInfo& scene)
	{
		qDebug()<<"[StoredViewDialog] Loading stored views for"<<scene.name;
		this->currentScene = scene;
		resetData(StoredView::getGlobalViewsForScene(currentScene),StoredView::getUserViewsForScene(currentScene));
		qDebug()<<"[StoredViewDialog]"<<rowCount(QModelIndex())<<"entries loaded";
	}

private:
	void resetData(const StoredViewList& global, const StoredViewList& user)
	{
		this->beginResetModel();
		this->global = global;
		this->user = user;
		this->endResetModel();
	}

	SceneInfo currentScene;
	StoredViewList global, user;
};


#endif
