#ifndef QUTE_NOTE_MODELS_NEW_ITEM_NAME_GENERATOR_HPP
#define QUTE_NOTE_MODELS_NEW_ITEM_NAME_GENERATOR_HPP

#include <QString>

namespace qute_note {

template <class NameIndexType>
QString newItemName(const NameIndexType & nameIndex, int & newItemCounter, QString baseName)
{
    if (newItemCounter != 0) {
        baseName += " (" + QString::number(newItemCounter) + ")";
    }

    while(true)
    {
        auto it = nameIndex.find(baseName.toUpper());
        if (it == nameIndex.end()) {
            return baseName;
        }

        if (newItemCounter != 0) {
            QString numPart = " (" + QString::number(newItemCounter) + ")";
            baseName.chop(numPart.length());
        }

        ++newItemCounter;
        baseName += " (" + QString::number(newItemCounter) + ")";
    }
}

} // namespace qute_note

#endif // QUTE_NOTE_MODELS_NEW_ITEM_NAME_GENERATOR_HPP
