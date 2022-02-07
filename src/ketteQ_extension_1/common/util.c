//
// Created by gchiappe on 2022-02-03.
//

#include "util.h"
#include "inttypes.h"

ptrdiff_t coutil_uint64_to_ptrdiff(uint64 input) {
    if (input > PTRDIFF_MAX) {
        return -1;
    }
    return (ptrdiff_t) input;
}

/**
 * From here: https://www.geeksforgeeks.org/binary-search/
 * @param arr
 * @param left
 * @param right
 * @param value
 * @return
 */
int32 coutil_binary_search(int32 arr[], int32 left, int32 right, int32 value) {
    if (right >= left) {
        int mid = left + (right - left) / 2;
        // If the element is present at the middle
        // itself
        if (arr[mid] == value)
            return mid;
        // If element is smaller than mid, then
        // it can only be present in left subarray
        if (arr[mid] > value)
            return coutil_binary_search(arr, left, mid - 1, value);
        // Else the element can only be present
        // in right subarray
        return coutil_binary_search(arr, mid + 1, right, value);
    }
    // We reach here when element is not
    // present in array
    return -1;
}