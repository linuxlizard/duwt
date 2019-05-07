#include <QtWidgets>

#include "logging.h"
#include "bssmodel.h"

BSSItem::BSSItem(const QList<QVariant>& data, BSSItem* parentItem)
{
}

BSSItem::~BSSItem()
{
}

// starting from Qt5.12 Example simpletreemodel
// see BSD License
BSSModel::BSSModel(QObject *parent)
	: QAbstractItemModel(parent)
{
    QList<QVariant> rootData;
    rootData << "Title" << "Summary";
    rootItem = new BSSItem(rootData);
	logger = spdlog::get("bssmodel");
	if (!logger) {
		logger = spdlog::stdout_logger_mt("bssmodel");
	}
	logger->info("hello from BSSModel");
}


BSSModel::~BSSModel()
{
	if (rootItem) {
		delete rootItem;
	}
}

Qt::ItemFlags BSSModel::flags(const QModelIndex &index) const
{
	logger->info("bssmodel flags() index={},{}", index.row(), index.column());

    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}

QVariant BSSModel::data(const QModelIndex &index, int role) const
{
	logger->info("bssmodel data() index={},{} role={}", 
			index.row(), index.column(), role);

    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

	if (index.column() > 0) {
		return QVariant(tr("Bar"));
	}
	return QVariant(tr("Foo"));

//    BSSItem *item = static_cast<BSSItem*>(index.internalPointer());

//    return item->data(index.column());
	return QVariant();
}

QVariant BSSModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
	logger->info("bssmodel headerData() section={}", section);

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
//        return rootItem->data(section);
        return QStringLiteral("Column %1").arg(section);
	}

    return QVariant();
}

int BSSModel::columnCount(const QModelIndex &parent) const
{
	logger->info("bssmodel columnCount() parent={},{}", parent.row(), parent.column());

	return 2;
}


int BSSModel::rowCount(const QModelIndex &parent) const
{
	logger->info("bssmodel rowCount() parent={},{}", parent.row(), parent.column());

	return 1;
}

QModelIndex BSSModel::index(int row, int column,
                  const QModelIndex& parent) const
{
	logger->info("bssmodel index() {},{}", row, column);

    if (!hasIndex(row, column, parent))
        return QModelIndex();

    BSSItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<BSSItem*>(parent.internalPointer());

    BSSItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex BSSModel::parent(const QModelIndex &index) const
{
	logger->info("bssmodel index() index={},{}", index.row(), index.column());

    if (!index.isValid())
        return QModelIndex();

    BSSItem *childItem = static_cast<BSSItem*>(index.internalPointer());
    BSSItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

