//
//  PropertiesTreeModel.cpp
//  UIEditor
//
//  Created by Dmitry Belsky on 12.9.14.
//
//

#include "PropertiesTreeModel.h"

#include "UIControls/BaseProperty.h"
#include "UIControls/UIEditorComponent.h"

#include <QPoint>
#include <QColor>
#include <QFont>
#include <QVector2D>
#include "Utils/QtDavaConvertion.h"

using namespace DAVA;

PropertiesTreeModel::PropertiesTreeModel(DAVA::UIControl *control, QObject *parent) : QAbstractItemModel(parent), root(NULL)
{
    UIEditorComponent* component = dynamic_cast<UIEditorComponent*>(control->GetCustomData());
    if (component)
        root = SafeRetain(component->GetPropertiesRoot());
}

PropertiesTreeModel::~PropertiesTreeModel()
{
    SafeRelease(root);
}

QModelIndex PropertiesTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();
    
    if (!parent.isValid())
        return createIndex(row, column, root->GetProperty(row));
    
    BaseProperty *property = static_cast<BaseProperty*>(parent.internalPointer());
    return createIndex(row, column, property->GetProperty(row));
}

QModelIndex PropertiesTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();
    
    BaseProperty *property = static_cast<BaseProperty*>(child.internalPointer());
    BaseProperty *parent = property->GetParent();
    
    if (parent == NULL || parent == root)
        return QModelIndex();

    if (parent->GetParent())
        return createIndex(parent->GetParent()->GetIndex(parent), 0, parent);
    else
        return createIndex(0, 0, parent);
}

int PropertiesTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    
    if (!parent.isValid())
        return root ? root->GetCount() : 0;
    
    return static_cast<BaseProperty*>(parent.internalPointer())->GetCount();
}

int PropertiesTreeModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid() || parent.internalPointer() == root)
        return 2;
    
    return 2;
}

QVariant PropertiesTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    
    BaseProperty *property = static_cast<BaseProperty*>(index.internalPointer());
    switch (role)
    {
        case Qt::CheckStateRole:
        {
            if (property->GetValue().GetType() == VariantType::TYPE_BOOLEAN && index.column() == 1)
                return property->GetValue().AsBool() ? Qt::Checked : Qt::Unchecked;
            break;
        }
            
        case Qt::DisplayRole:
        case Qt::EditRole:
        {
            if (index.column() == 0)
                return property->GetName().c_str();
            if (role == Qt::DisplayRole)
            {
                return makeQString(property);
            }
            else
            {
                return makeQVariant(property->GetValue());
            }
        }
        break;
            
        case Qt::BackgroundRole:
            return property->GetType() == BaseProperty::TYPE_HEADER ? Qt::lightGray : Qt::white;
            
        case Qt::FontRole:
        {
            if (property->IsReplaced())
            {
                QFont myFont;
                myFont.setBold(true);
                return myFont;
            }
            return QVariant();
        }
    }
    
    return QVariant();
}

bool PropertiesTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    BaseProperty *property = static_cast<BaseProperty*>(index.internalPointer());
    switch (role)
    {
    case Qt::CheckStateRole:
        {
            if (property->GetValue().GetType() == VariantType::TYPE_BOOLEAN)
            {
                property->SetValue(VariantType(value != Qt::Unchecked));
                return true;
            }
        }
        break;
    case Qt::EditRole:
        {
            VariantType newVal(property->GetValue());
            initVariantType(newVal, value);
            property->SetValue(newVal);
            return true;
        }
        break;
    }
    return false;
}
Qt::ItemFlags PropertiesTreeModel::flags(const QModelIndex &index) const
{
    if (index.column() != 1)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    
    BaseProperty* prop = static_cast<BaseProperty*>(index.internalPointer());
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
    if (prop->GetType() == BaseProperty::TYPE_ENUM || prop->GetType() == BaseProperty::TYPE_FLAGS || prop->GetType() == BaseProperty::TYPE_VARIANT)
        flags |= Qt::ItemIsEditable;
    return flags;
}

QVariant PropertiesTreeModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (section == 0)
            return "Property";
        else
            return "Value";
    }
    return QVariant();
}

QVariant PropertiesTreeModel::makeQVariant(const VariantType &val) const
{
    switch (val.GetType())
    {
        case VariantType::TYPE_NONE:
            return QVariant();
            
        case VariantType::TYPE_BOOLEAN:
            return val.AsBool();
            
        case VariantType::TYPE_INT32:
            return val.AsInt32();
            
        case VariantType::TYPE_FLOAT:
            return val.AsFloat();

        case VariantType::TYPE_STRING:
            return StringToQString(val.AsString());
            
        case VariantType::TYPE_WIDE_STRING:
            return WideStringToQString(val.AsWideString());
            
        case VariantType::TYPE_UINT32:
            return val.AsUInt32();

        case VariantType::TYPE_INT64:
            return val.AsInt64();
            
        case VariantType::TYPE_UINT64:
            return val.AsUInt64();

        case VariantType::TYPE_VECTOR2:
            return Vector2ToQVector2D(val.AsVector2());

        case VariantType::TYPE_COLOR:
            return ColorToQColor(val.AsColor());

        case VariantType::TYPE_BYTE_ARRAY:
        case VariantType::TYPE_KEYED_ARCHIVE:
        case VariantType::TYPE_VECTOR3:
        case VariantType::TYPE_VECTOR4:
        case VariantType::TYPE_MATRIX2:
        case VariantType::TYPE_MATRIX3:
        case VariantType::TYPE_MATRIX4:
        case VariantType::TYPE_FASTNAME:
        case VariantType::TYPE_AABBOX3:
        case VariantType::TYPE_FILEPATH:
        default:
            DVASSERT(false);
            break;
    }
    return QVariant();
}

DAVA::VariantType PropertiesTreeModel::makeVariantType(const QVariant & /*val*/) const
{
    DVASSERT(false);
    return VariantType();
}

QString PropertiesTreeModel::makeQString(const BaseProperty *property) const
{
    const VariantType &val = property->GetValue();
    switch (val.GetType())
    {
        case VariantType::TYPE_NONE:
            return QString();
            
        case VariantType::TYPE_BOOLEAN:
            return QVariant(val.AsBool()).toString();
            
        case VariantType::TYPE_INT32:
            if (property->GetType() == BaseProperty::TYPE_ENUM)
            {
                int32 e = val.AsInt32();
                return QString::fromStdString(property->GetEnumMap()->ToString(e));
            }
            else if (property->GetType() == BaseProperty::TYPE_FLAGS)
            {
                int32 e = val.AsInt32();
                QString res = "";
                int p = 0;
                while (e)
                {
                    if ((e & 0x01) != 0)
                    {
                        if (!res.isEmpty())
                            res += " | ";
                        res += QString::fromStdString(property->GetEnumMap()->ToString(1 << p));
                    }
                    p++;
                    e >>= 1;
                }
                return res;
            }
            else
            {
                return QVariant(val.AsInt32()).toString();
            }
            
        case VariantType::TYPE_FLOAT:
            return QVariant(val.AsFloat()).toString();
            
        case VariantType::TYPE_STRING:
            return StringToQString(val.AsString());
            
        case VariantType::TYPE_WIDE_STRING:
            return WideStringToQString(val.AsWideString());
            
//        case VariantType::TYPE_UINT32:
//            return val.AsUInt32();
//            
//        case VariantType::TYPE_INT64:
//            return val.AsInt64();
//            
//        case VariantType::TYPE_UINT64:
//            return val.AsUInt64();
            
        case VariantType::TYPE_VECTOR2:
            return QString::fromStdString(Format("[%g, %g]", val.AsVector2().x, val.AsVector2().y));
            
        case VariantType::TYPE_COLOR:
            return QString::fromStdString(Format("%g, %g, %g, %g", val.AsColor().a, val.AsColor().r, val.AsColor().g, val.AsColor().b));
            
        case VariantType::TYPE_BYTE_ARRAY:
        case VariantType::TYPE_KEYED_ARCHIVE:
        case VariantType::TYPE_VECTOR3:
        case VariantType::TYPE_VECTOR4:
        case VariantType::TYPE_MATRIX2:
        case VariantType::TYPE_MATRIX3:
        case VariantType::TYPE_MATRIX4:
        case VariantType::TYPE_FASTNAME:
        case VariantType::TYPE_AABBOX3:
        case VariantType::TYPE_FILEPATH:
        default:
            DVASSERT(false);
            break;
    }
    return QString();
}

void PropertiesTreeModel::initVariantType(DAVA::VariantType &var, const QVariant &val) const
{
    switch (var.GetType())
    {
        case VariantType::TYPE_NONE:
            break;
            
        case VariantType::TYPE_BOOLEAN:
            var.SetBool(val.toBool());
            break;
            
        case VariantType::TYPE_INT32:
            var.SetInt32(val.toInt());
            break;
            
        case VariantType::TYPE_FLOAT:
            var.SetFloat(val.toFloat());
            break;
            
        case VariantType::TYPE_STRING:
            var.SetString(val.toString().toStdString());
            break;
            
        case VariantType::TYPE_WIDE_STRING:
            var.SetWideString(QStringToWideString(val.toString()));
            break;
            
//        case VariantType::TYPE_UINT32:
//            return val.AsUInt32();
//            
//        case VariantType::TYPE_INT64:
//            return val.AsInt64();
//            
//        case VariantType::TYPE_UINT64:
//            return val.AsUInt64();
            
        case VariantType::TYPE_VECTOR2:
        {
            QVector2D vector = val.value<QVector2D>();
            var.SetVector2(DAVA::Vector2(vector.x(), vector.y()));
        }
            break;
            
        case VariantType::TYPE_COLOR:
            //return QString::fromStdString(Format("%.3f, %.3f, %.3f, %.3f", val.AsColor().a, val.AsColor().r, val.AsColor().g, val.AsColor().b));
            break;
            
//        case VariantType::TYPE_BYTE_ARRAY:
//        case VariantType::TYPE_KEYED_ARCHIVE:
//        case VariantType::TYPE_VECTOR3:
//        case VariantType::TYPE_VECTOR4:
//        case VariantType::TYPE_MATRIX2:
//        case VariantType::TYPE_MATRIX3:
//        case VariantType::TYPE_MATRIX4:
//        case VariantType::TYPE_FASTNAME:
//        case VariantType::TYPE_AABBOX3:
//        case VariantType::TYPE_FILEPATH:
        default:
            DVASSERT(false);
            break;
    }
}