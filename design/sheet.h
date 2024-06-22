#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

class Sheet : public SheetInterface {
public:
    Sheet();
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_; //row - column

    Size GetLimits() const;

    std::map<int, int> col_fill_count;
    std::map<int, int> row_fill_count;
};