#pragma once

#include <QAbstractListModel>
#include "core\Correction.h"

class CorrectionModel  : public QAbstractListModel
{
	Q_OBJECT

public:
	CorrectionModel(QObject *parent);
	~CorrectionModel();

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index,
		int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex& index,
		const QVariant& value,
		int role) override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;

	void setCorrections(const QVector<Correction>& list);
	QVector<Correction> enabledCorrections() const;

	std::array<double, 6> correctionRangeArray(int row) const;

	QVector<Correction>& getItems() { return m_items; }

	QVector<Correction> corrections() const { return m_items; }

private:
	QVector<Correction> m_items;
};

