// @ref https://github.com/hauptmech/eigen-initializer_list

// @ref ../cpp_quick/composition_ctor.cc
// @ref ./matrix_inheritance.cc

// Purpose: For fun

/*
Support matrix concatenation for a matrix such as:

  -------------
  | A |   | D |
  |---| C |---|
  | B |   | E |
  |-----------|
  |     F     |
  -------------

Achievable via:

    MATLAB:
    x =  [ [[A; B], C], [D; E];
                   F        ];

    Numpy:
    x = vstack(
            hstack( vstack(A, B), C, vstack(E, F) ),
            F
        )

    Possible? with initializer lists:
       {
         { {{A}, {B}}, C, {{D}, {E}} },
         { F} }
       }

    Grammar:

        initializer_list<Init> ==> hstack
        initializer_list<initializer_list<Init>> ==> vstack

    Achievable with composition construction (see ../composition_ctor.cc)

    Challenge: Defer evaluation until the end, do things efficiently, etc.
        Will figure that out later

*/

#include "cpp_quick/name_trait.h"

#include <string>
#include <iostream>

#include <Eigen/Dense>

using std::cout;
using std::endl;
using std::string;

class MatrixXc : public Eigen::Matrix<string, Eigen::Dynamic, Eigen::Dynamic> {
public:
    using Matrix::Matrix;

    using col_initializer_list = std::initializer_list<MatrixXc>;
    using row_initializer_list = std::initializer_list<col_initializer_list>;

    MatrixXc(row_initializer_list row_list) {
        int rows = 0;
        int cols = -1;

        // Get variable for clarity
        auto& X = derived();

        // First review size
        for (const auto& col_list : row_list) {
            // Construct row:
            // @require: All items have same number of rows
            // @require: Columns be equal to overall # of columns
            int row_rows = -1;
            int row_cols = 0;
            for (const auto& item : col_list) {
                int item_rows = item.rows();
                int item_cols = item.cols();
                if (row_rows == -1)
                    row_rows = item_rows;
                else
                    // Already set, must match
                    eigen_assert(item_rows == row_rows);
                row_cols += item_cols;
            }
            if (cols == -1)
                cols = row_cols;
            else
                // Already set, must match
                eigen_assert(row_cols == cols);
            rows += row_rows;
        }

        if (cols == -1)
            // Ensure that we have a valid size
            cols = 0;

        // We now have our desired size
        resize(rows, cols);

        // Now fill in the data
        // We know that our data is good, no further checks needed
        int r = 0;
        for (const auto& col_list : row_list) {
            int c = 0;
            int row_rows = 0;
            for (const auto& item : col_list) {
                int item_rows = item.rows();
                int item_cols = item.cols();
                X.block(r, c, item_rows, item_cols) = item;
                row_rows = item_rows;
                c += item_cols;
            }
            r += row_rows;
        }
    }
};

void fill(MatrixXc& X, string prefix) {
    static const string hex = "01234566789abcdef";
    for (int i = 0; i < X.size(); ++i)
        X(i) = prefix + "[" + hex[i] + "] ";
}

int main() {
    MatrixXc A(1, 2), B(1, 2),
        C(2, 1),
        D(1, 3), E(1, 3),
        F(2, 6);
    fill(A, "A");
    fill(B, "B");
    fill(C, "C");
    fill(D, "D");
    fill(E, "E");
    fill(F, "F");

    cout
        << "A: " << endl << A << endl << endl
        << "B: " << endl << B << endl << endl
        << "C: " << endl << C << endl << endl
        << "D: " << endl << D << endl << endl
        << "E: " << endl << E << endl << endl
        << "F: " << endl << F << endl << endl;

    MatrixXc X = {
            { {{A}, {B}}, C, {{D}, {E}} },
            { F }
        };

    cout
        << "X: " << endl << X << endl << endl;

    return 0;
}
