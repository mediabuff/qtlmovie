//----------------------------------------------------------------------------
//
// Copyright (c) 2013, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
// Qtl, Qt utility library.
// Definition of utilities for QTableWidget.
//
//----------------------------------------------------------------------------

#include "QtlTableWidgetUtils.h"


//-----------------------------------------------------------------------------
// Set a header in a QTableWidget.
//-----------------------------------------------------------------------------

QTableWidgetItem* qtlSetTableHorizontalHeader(QTableWidget* table, int column, const QString& text, int alignment)
{
    if (table == 0) {
        return 0;
    }
    else {
        QTableWidgetItem* item = new QTableWidgetItem(text);
        item->setTextAlignment(alignment);
        table->setHorizontalHeaderItem(column, item);
        return item;
    }
}


//-----------------------------------------------------------------------------
// Remove all items in a row of a QTableWidget and return them.
//-----------------------------------------------------------------------------

QList<QTableWidgetItem*> qtlTakeTableRow(QTableWidget* table, int row)
{
    QList<QTableWidgetItem*> items;
    if (table != 0 && row >= 0 && row < table->rowCount()) {
        for (int column = 0; column < table->columnCount(); ++column) {
            items << table->takeItem(row, column);
        }
    }
    return items;
}


//-----------------------------------------------------------------------------
// Set all items in a row of a QTableWidget.
//-----------------------------------------------------------------------------

void qtlSetTableRow(QTableWidget* table, int row, const QList<QTableWidgetItem*>& items)
{
    if (table != 0 && !items.isEmpty() && row >= 0) {
        for (int column = 0; column < items.size(); ++column) {
            QTableWidgetItem* const item = items[column];
            if (item != 0) {
                table->setItem(row, column, item);
                // The row may have changed if automatic sorting was enabled.
                row = item->row();
            }
        }
    }
}
