#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <list>
#include <optional>
#include <set>

using namespace std::literals;

const int Position::MAX_ROWS;
const int Position::MAX_COLS;

Sheet::Sheet() {
    sheet_.resize(200);
    for (auto& row : sheet_) {
        row.resize(200);
    }
}

bool IsInvalidPos(Position pos) {
    return pos.col < 0 || pos.col >= pos.MAX_COLS || pos.row < 0 || pos.row >= pos.MAX_ROWS;
}

void CopyCellDependencedList(const Cell* from, Cell* to) {
    to->SetDependencedList(from);
}

void Sheet::ResizeSheet(Position pos) {
    if (pos.row >= static_cast<int>(sheet_.size())) {
        sheet_.resize(std::min(pos.row * 2, pos.MAX_ROWS));
        int new_col_size = std::max(pos.col, static_cast<int>(sheet_.at(0).size()));
        for (auto& row : sheet_) {
            row.resize(std::min(new_col_size * 2, pos.MAX_COLS));
        }
    }

    if (pos.col >= static_cast<int>(sheet_.at(0).size())) {
        for (auto& row : sheet_) {
            row.resize(std::min(pos.col * 2, pos.MAX_COLS));
        }
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    if (IsInvalidPos(pos))
        throw InvalidPositionException("Invalid position of cell"s);

    ResizeSheet(pos);

    std::unique_ptr<Cell> temp_cell = std::make_unique<Cell>();
    temp_cell->Set(text, *this);
    
    if (std::vector<Position> refs = temp_cell->GetReferencedCells(); !refs.empty()) {
        if (HasCircularDependences(pos, refs)) throw CircularDependencyException("Has circular dependency");
    }
    if (GetCell(pos)) {
        CopyCellDependencedList(static_cast<Cell*>(GetCell(pos)), temp_cell.get());
    }
    temp_cell->InvalidateCache();

    sheet_[pos.row][pos.col].reset(temp_cell.release());
    
    ++row_fill_count[pos.row];
    ++col_fill_count[pos.col];
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (IsInvalidPos(pos))
        throw InvalidPositionException("Invalid position of cell"s);

    if (pos.row > static_cast<int>(sheet_.size())
        || pos.col > static_cast<int>(sheet_.at(0).size())
        || sheet_.empty())
        return nullptr;

    return sheet_.at(pos.row).at(pos.col).get();
}
CellInterface* Sheet::GetCell(Position pos) {
    if (IsInvalidPos(pos))
        throw InvalidPositionException("Invalid position of cell"s);

    if (pos.row > static_cast<int>(sheet_.size())
        || pos.col > static_cast<int>(sheet_.at(0).size())
        || sheet_.empty())
        return nullptr;

    return sheet_.at(pos.row).at(pos.col).get();
}

void Sheet::ClearCell(Position pos) {
    if (IsInvalidPos(pos))
        throw InvalidPositionException("Invalid position of cell"s);

    if (pos.row > static_cast<int>(sheet_.size())
        || pos.col > static_cast<int>(sheet_.at(0).size())
        || sheet_.empty())
        return;

    if (!sheet_.at(pos.row).at(pos.col)) return;

    sheet_.at(pos.row).at(pos.col)->Clear();
    sheet_.at(pos.row).at(pos.col).reset(nullptr);
    --row_fill_count.at(pos.row);
    --col_fill_count.at(pos.col);

    if (row_fill_count.at(pos.row) == 0) row_fill_count.erase(pos.row);
    if (col_fill_count.at(pos.col) == 0) col_fill_count.erase(pos.col);
}

Size Sheet::GetLimits() const {
    if (row_fill_count.empty()) return { -1, -1 };

    auto last_col_iter = --col_fill_count.end();
    int width = last_col_iter->first;
    auto last_row_iter = --row_fill_count.end();
    int hight = last_row_iter->first;
    return{ hight, width };
}

Size Sheet::GetPrintableSize() const {
    Size limits = GetLimits();

    return { limits.rows + 1, limits.cols + 1 };
}

void Sheet::PrintValues(std::ostream& output) const {
    Size print_size = GetLimits();
    if (print_size.rows == -1) return;

    bool is_first_in_row = true;
    for (int row = 0; row <= print_size.rows; ++row) {

        for (int col = 0; col <= print_size.cols; ++col) {
            if (sheet_.at(row).at(col)) {
                if (!is_first_in_row) output << '\t';
                CellInterface::Value value = sheet_.at(row).at(col)->GetValue();

                switch (value.index()) {
                case 0: {
                    output << std::get<0>(value);
                    break;
                }
                case 1: {
                    output << std::get<1>(value);
                    break;
                }
                case 2: {
                    output << std::get<2>(value).ToString();
                    break;
                }
                default:
                    assert(false);
                }
                /*
                if (std::holds_alternative<std::string>(value)) output << std::get<0>(value);
                else if (std::holds_alternative<double>(value)) output << std::get<1>(value);
                else output << "#ARITHM!";
                */
                is_first_in_row = false;
            }
            else {
                if (!is_first_in_row) {
                    output << '\t';
                }
                is_first_in_row = false;
            }
        }
        output << '\n';
        is_first_in_row = true;
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size print_size = GetLimits();
    if (print_size.rows == -1) return;

    bool is_first_in_row = true;
    for (int row = 0; row <= print_size.rows; ++row) {

        for (int col = 0; col <= print_size.cols; ++col) {
            if (sheet_.at(row).at(col)) {
                if (!is_first_in_row) {
                    output << '\t';
                }

                output << sheet_.at(row).at(col)->GetText();
                is_first_in_row = false;
            }
            else {
                if (!is_first_in_row) {
                    output << '\t';
                }
                is_first_in_row = false;
            }
        }
        output << '\n';
        is_first_in_row = true;
    }
}

bool Sheet::HasCircularDependences(const Position& pos, const std::vector<Position>& dependences_list) const {
    if (dependences_list.empty())
        return false;
    std::set<Position> checked_cells;
    std::list<Position> queue_to_visit;

    for (const Position& depend : dependences_list) {

        queue_to_visit.push_back(depend);

        while (!queue_to_visit.empty()) {

            const Position& front = queue_to_visit.front();
            if (GetCell(front)) {

                for (const Position& subdepence : GetCell(front)->GetReferencedCells()) {
                    if (checked_cells.count(subdepence) == 0) {
                        queue_to_visit.push_back(subdepence);
                        if (subdepence == pos)
                            return true;
                    }
                }
            }

            checked_cells.insert(front);
            queue_to_visit.pop_front();
        }
    }
    return (checked_cells.count(pos) > 0);
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}