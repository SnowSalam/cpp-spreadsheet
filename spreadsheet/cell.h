#pragma once

#include <optional>
#include <functional>
#include <unordered_set>

#include "common.h"
#include "formula.h"

class Cell : public CellInterface {
public:
    Cell();

    void Set(std::string text, SheetInterface& sheet);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    void InvalidateCache() const;

    void SetDependencedList(const Cell* other_cell);

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;
    mutable std::optional<Value> cache_;

    void UpdateDependencies(const std::vector<Position>& new_ref_cells, SheetInterface& sheet);
    void ClearReferencies();

    std::unordered_set<Cell*> dependenced_list_;
    std::unordered_set<Cell*> referenced_list_;
};

class Cell::Impl {
public:
    using Value = CellInterface::Value;
    virtual ~Impl() = default;

    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;

    virtual std::vector<Position> GetReferencedCells() const {
        return {};
    }
};