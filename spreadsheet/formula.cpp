#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <forward_list>
#include <set>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression) try : ast_(ParseFormulaAST(expression)) {
        }
        catch (const std::exception& exc) {
            std::throw_with_nested(FormulaException(exc.what()));
        }

        Value Evaluate(const SheetInterface& sheet) const override {
            Value result;
            try {
                result = ast_.Execute(sheet);
            }
            catch (const FormulaError& err) {
                result = err;
            }
            return result;
        }

        std::string GetExpression() const override {
            std::stringstream oss;
            ast_.PrintFormula(oss);
            return oss.str();
        }

        std::vector<Position> GetReferencedCells() const override {
            const std::forward_list<Position>& pos_list = ast_.GetCells();
            std::set<Position> cells(pos_list.begin(), pos_list.end());
            return { cells.begin(), cells.end() };
        }

    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}