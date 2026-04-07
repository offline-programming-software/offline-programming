#pragma once
#include "OnlineProcess/connectSetting.h"
#include <qabstractitemmodel.h>

class ClientModel: public QAbstractItemModel
{
public:
	ClientModel(QObject* parent = nullptr);
	~ClientModel();
	// QAbstractItemModel 必需接口
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	// 自定义接口
	void addClient(const Client& client);
	void removeClient(int row);
	void updateClient(int row, const Client& client);

	QVector<Client>& getClients() { return m_clients; }
	const Client& getClient(int row) const { return m_clients.at(row); }

private:
	QVector<Client> m_clients;

};

