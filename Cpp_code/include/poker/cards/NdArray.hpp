#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace poker::cards {

    class Array2D {
    public:
        Array2D() = default;
        Array2D(std::size_t rows, std::size_t cols) : rows_(rows), cols_(cols), data_(rows * cols, 0) {}

        int& operator()(std::size_t r, std::size_t c) { return data_[r * cols_ + c]; }
        int operator()(std::size_t r, std::size_t c) const { return data_[r * cols_ + c]; }
        int* row(std::size_t r) { return data_.data() + r * cols_; }
        const int* row(std::size_t r) const { return data_.data() + r * cols_; }

        std::size_t rows() const { return rows_; }
        std::size_t cols() const { return cols_; }
        const std::vector<int>& data() const { return data_; }

    private:
        std::size_t rows_ = 0, cols_ = 0;
        std::vector<int> data_;
    };

    class Array3D {
    public:
        Array3D() = default;
        Array3D(std::size_t depth, std::size_t rows, std::size_t cols)
            : depth_(depth), rows_(rows), cols_(cols), data_(depth * rows * cols, 0) {}

        int& operator()(std::size_t d, std::size_t r, std::size_t c) { return data_[(d * rows_ + r) * cols_ + c]; }
        int operator()(std::size_t d, std::size_t r, std::size_t c) const { return data_[(d * rows_ + r) * cols_ + c]; }
        int* hand(std::size_t d, std::size_t r) { return data_.data() + (d * rows_ + r) * cols_; }
        const int* hand(std::size_t d, std::size_t r) const { return data_.data() + (d * rows_ + r) * cols_; }

        std::size_t depth() const { return depth_; }
        std::size_t rows() const { return rows_; }
        std::size_t cols() const { return cols_; }
        const std::vector<int>& data() const { return data_; }

    private:
        std::size_t depth_ = 0, rows_ = 0, cols_ = 0;
        std::vector<int> data_;
    };

}
