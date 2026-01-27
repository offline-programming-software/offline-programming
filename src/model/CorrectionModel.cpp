#include "CorrectionModel.h"

CorrectionModel::CorrectionModel(QObject *parent)
	: QAbstractListModel(parent)
{}

CorrectionModel::~CorrectionModel()
{}

int CorrectionModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
	return m_items.size();
}

/**
 * @fn data
 * 
 * @brief .
 * 
 */
QVariant CorrectionModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return {};

	const auto& c = m_items.at(index.row());

	if (role == Qt::DisplayRole)
		return c.name();

	if (role == Qt::CheckStateRole)
		return c.isApplied() ? Qt::Checked : Qt::Unchecked;

	return {};
}

	bool CorrectionModel::setData(const QModelIndex& index,
		const QVariant& value,
		int role)
	{
		if (!index.isValid())
			return false;

		if (role == Qt::CheckStateRole)
		{
			m_items[index.row()].setIsApply((value.toInt() == Qt::Checked));

			emit dataChanged(index, index);
			return true;
		}
		return false;
	}

Qt::ItemFlags CorrectionModel::flags(const QModelIndex& index) const
{
	return Qt::ItemIsEnabled |
		Qt::ItemIsSelectable |
		Qt::ItemIsUserCheckable;
}

void CorrectionModel::setCorrections(const QVector<Correction>& list)
{
	
	beginResetModel();
	m_items = list;
	endResetModel();
}

QVector<Correction> CorrectionModel::enabledCorrections() const
{
	QVector<Correction> result;
	for (const auto& c : m_items)
		if (c.isApplied())
			result.append(c);
	return result;
}

std::array<double, 6> CorrectionModel::correctionRangeArray(int row) const
{
	Q_ASSERT(row >= 0 && row < m_items.size());
	std::array<double, 6> arr;
	std::copy(std::begin(m_items[row].m_range), std::end(m_items[row].m_range), arr.begin());
	return arr;
}

