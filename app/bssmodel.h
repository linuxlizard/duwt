#ifndef BSSMODEL_H
#define BSSMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#include "logging.h"

class BSSItem
{
public:
    explicit BSSItem(const QList<QVariant> &data, BSSItem *parentItem = nullptr);
    ~BSSItem();

    BSSItem *parentItem() { return m_parentItem; }
    BSSItem *child(int row) { return nullptr; }
    int row() const { return 0; }

private:
    BSSItem *m_parentItem;
};

// https://doc.qt.io/qt-5/model-view-programming.html#model-subclassing-reference
class BSSModel : public QAbstractItemModel
{
	Q_OBJECT

	public:
        explicit BSSModel(QObject *parent = nullptr);
        virtual ~BSSModel() override;

		// starting from Qt5.12 Example simpletreemodel
		// see BSD License
        virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

        virtual QVariant data(const QModelIndex &index, int role) const override;

        virtual QVariant headerData(int section, Qt::Orientation orientation,
							int role = Qt::DisplayRole) const override;

        virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

        virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;

        virtual QModelIndex index(int row, int column,
                          const QModelIndex &parent = QModelIndex()) const override;

        virtual QModelIndex parent(const QModelIndex &index) const override;

	private:
		BSSItem *rootItem;

		std::shared_ptr<spdlog::logger> logger;
};

#endif

