#pragma once

#include <optional>

#include "common.h"
#include "formula.h"

class Cell : public CellInterface {
public:
    Cell();
    ~Cell();

    void Set(std::string text, const SheetInterface& sheet);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    void DeleteCache_();
    bool HasCache() const;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;

    std::vector<Position> dependency_list_;
    std::vector<Position> referency_list_;
};

class Cell::Impl {
public:
    using Value = CellInterface::Value;
    Impl() = default;

    virtual Value GetValue()  const = 0;
    virtual std::string GetText()  const = 0;
};

class Cell::EmptyImpl : public Impl {
public:
    Value GetValue() const override;
    std::string GetText() const override;
};

class Cell::TextImpl : public Impl {
public:
    TextImpl(std::string& text) : text_(text) {}

    Value GetValue() const override;
    std::string GetText() const override;
private:
    std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    FormulaImpl(std::string& formula, const SheetInterface& sheet) : formula_(ParseFormula(formula.substr(1)))
        , sheet_(&sheet) {}

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const;
    std::optional<FormulaInterface::Value> GetCache() const;
    void DeleteCache();
private:
    std::unique_ptr<FormulaInterface> formula_;
    const SheetInterface* sheet_;
    mutable std::optional<FormulaInterface::Value> cache_;
};