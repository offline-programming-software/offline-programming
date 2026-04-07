#include "OnlineProcess/model/clientModel.h"

ClientModel::ClientModel(QObject* parent)
	: QAbstractItemModel(parent)
{
}

ClientModel::~ClientModel()
{
}

int ClientModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;
	return m_clients.size();
}

int ClientModel::columnCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;
	return 5; // name, ip, port, isLocal, status
}

QVariant ClientModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_clients.size())
		return QVariant();

	const Client& client = m_clients.at(index.row());
	int column = index.column();

	if (role == Qt::DisplayRole || role == Qt::EditRole)
	{
		switch (column)
		{
		case 0: return client.name;
		case 1: return client.ip;
		case 2: return client.port;
		case 3: return client.isLocal ? "本地" : "远程";
		case 4: return client.status ? "在线" : "离线";
		default: return QVariant();
		}
	}

	if (role == Qt::CheckStateRole && column == 3)
	{
		return client.isLocal ? Qt::Checked : Qt::Unchecked;
	}

	return QVariant();
}

QVariant ClientModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		switch (section)
		{
		case 0: return QString::fromLocal8Bit("客户端名称");
		case 1: return QString::fromLocal8Bit("IP地址");
		case 2: return QString::fromLocal8Bit("端口");
		case 3: return QString::fromLocal8Bit("连接方式");
		case 4: return QString::fromLocal8Bit("状态");
		default: return QVariant();
		}
	}
	return QVariant();
}

Qt::ItemFlags ClientModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	if (index.column() == 0 || index.column() == 1 || index.column() == 2)
		f |= Qt::ItemIsEditable;

	return f;
}

bool ClientModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_clients.size())
		return false;

	Client& client = m_clients[index.row()];
	int column = index.column();

	if (role == Qt::EditRole)
	{
		switch (column)
		{
		case 0: client.name = value.toString(); break;
		case 1: client.ip = value.toString(); break;
		case 2: client.port = value.toString(); break;
		default: return false;
		}
		emit dataChanged(index, index);
		return true;
	}

	if (role == Qt::CheckStateRole && column == 3)
	{
		client.isLocal = (value.toInt() == Qt::Checked);
		emit dataChanged(index, index);
		return true;
	}

	return false;
}

void ClientModel::addClient(const Client& client)
{
	beginInsertRows(QModelIndex(), m_clients.size(), m_clients.size());
	m_clients.push_back(client);
	endInsertRows();
}

void ClientModel::removeClient(int row)
{
	if (row < 0 || row >= m_clients.size())
		return;

	beginRemoveRows(QModelIndex(), row, row);
	m_clients.removeAt(row);
	endRemoveRows();
}

void ClientModel::updateClient(int row, const Client& client)
{
	if (row < 0 || row >= m_clients.size())
		return;

	m_clients[row] = client;
	QModelIndex idx = index(row, 0);
	emit dataChanged(idx, index(row, columnCount() - 1));
}