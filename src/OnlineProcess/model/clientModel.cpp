#include "OnlineProcess/model/clientModel.h"

ClientModel::ClientModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

ClientModel::~ClientModel()
{
}

int ClientModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return m_clients.count();
}

int ClientModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return ColumnCount;
}

QVariant ClientModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_clients.count() || index.row() < 0)
        return QVariant();

    const Client& client = m_clients.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColName:
            return client.name();
        case ColIp:
            if (client.ip() == "...")
            {
                return QStringLiteral("本地连接");
            }
            else
            {
				return client.ip();
            }
        case ColPort:
            return client.port();
        case ColConnected:
            return client.getIsConnected() ? QStringLiteral("已连接") : QStringLiteral("未连接");
        default:
            return QVariant();
        }
    }
    else if (role == Qt::TextAlignmentRole) {
        return Qt::AlignCenter;
    }

    return QVariant();
}

QVariant ClientModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case ColName:
            return QStringLiteral("名称");
        case ColIp:
            return QStringLiteral("IP地址");
        case ColPort:
            return QStringLiteral("端口号");
        case ColConnected:
            return QStringLiteral("连接状态");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags ClientModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractTableModel::flags(index);
}

QVector<Client> ClientModel::getAllClient() const
{
    return m_clients;
}

Client& ClientModel::getClient(int row)
{
    return m_clients[row];
}

void ClientModel::setClients(const QVector<Client>& clients)
{
    beginResetModel();
    m_clients = clients;
    endResetModel();
}

void ClientModel::addClient(const Client& client)
{
    int newRowPosition = m_clients.count();

    beginInsertRows(QModelIndex(), newRowPosition, newRowPosition);
    m_clients.append(client);
    endInsertRows();
}

void ClientModel::clear()
{
    beginResetModel();
    m_clients.clear();
    endResetModel();
}