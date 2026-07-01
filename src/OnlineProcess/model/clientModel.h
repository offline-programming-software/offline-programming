#pragma once
#include "OnlineProcess/Client.h"
#include <QAbstractTableModel>
#include <QVector>

class ClientModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ClientModel(QObject* parent = nullptr);
    ~ClientModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
	QVector<Client> getAllClient() const;
    Client& getClient(int row);

    void setClients(const QVector<Client>& clients);
    void addClient(const Client& client);
    void clear();

private:
    QVector<Client> m_clients;

    // 根据 Client 的属性调整列枚举
    enum Column {
        ColName = 0,
        ColIp,
        ColPort,
        ColConnected,
        ColumnCount
    };
};