#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

class Cell::EmptyImpl : public Impl {
public:
    Value GetValue() const override {
        return "";
    }
    std::string GetText() const override {
        return "";
    }
};

class Cell::TextImpl : public Impl {
public:
    TextImpl(std::string& text) : text_(text) {}

    Value GetValue() const override {
        if (text_[0] == '\'') {
            if (text_.size() == 1) return "";
            return text_.substr(1);
        }
        return text_;
    }

    std::string GetText() const override {
        return text_;
    }

private:
    std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    FormulaImpl(std::string& formula, const SheetInterface& sheet)
        : formula_(ParseFormula(formula.substr(1)))
        , sheet_(&sheet) {}

    Value GetValue() const override {
        FormulaInterface::Value value = formula_->Evaluate(*sheet_);
        if (std::holds_alternative<double>(value)) return std::get<double>(value);
        return std::get<FormulaError>(value);
    }

    std::string GetText() const override {
        return "=" + formula_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }

private:
    std::unique_ptr<FormulaInterface> formula_;
    const SheetInterface* sheet_;
};

Cell::Cell() : impl_(std::make_unique<EmptyImpl>()) {}

void Cell::SetDependencedList(const Cell* other_cell) {
    dependenced_list_ = other_cell->dependenced_list_;
}

void Cell::UpdateDependencies(const std::vector<Position>& new_ref_cells, SheetInterface& sheet) {
    if (!referenced_list_.empty()) {
        ClearReferencies();
    }

    for (const Position& pos : new_ref_cells) {
        if (!sheet.GetCell(pos)) {
            sheet.SetCell(pos, "");
        }
        Cell* new_referenced_cell = static_cast<Cell*>(sheet.GetCell(pos));

        referenced_list_.insert(new_referenced_cell);
        new_referenced_cell->dependenced_list_.insert(this);
    }
}

void Cell::Set(std::string text, SheetInterface& sheet) {
    if (text.empty()) {
        Clear();
        return;
    }

    if (text[0] == '=' && text.size() != 1) {
        std::unique_ptr<Impl> temp_impl = std::make_unique<FormulaImpl>(text, sheet);
        const std::vector<Position> new_referenced_cells = temp_impl->GetReferencedCells();
        if (!new_referenced_cells.empty()) {
            UpdateDependencies(new_referenced_cells, sheet);
        }
        impl_ = std::move(temp_impl);
    }
    else {
        impl_ = std::make_unique<TextImpl>(text);
    }

    InvalidateCache();
}

void Cell::ClearReferencies() {
    for (Cell* reference_cell : referenced_list_) {
        reference_cell->dependenced_list_.erase(this);
    }
    referenced_list_.clear();
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
    InvalidateCache();
    ClearReferencies();
}

Cell::Value Cell::GetValue() const {
    if (cache_.has_value()) return cache_.value();

    if (impl_ != nullptr) {
        cache_ = impl_->GetValue();
        return cache_.value();
    }

    return Value();
}
std::string Cell::GetText() const {
    return impl_->GetText();
}

void Cell::InvalidateCache() const {
    cache_.reset();

    for (const Cell* dependency : dependenced_list_) {
        dependency->InvalidateCache();
    }
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}