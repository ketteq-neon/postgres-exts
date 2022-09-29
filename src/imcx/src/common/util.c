/**
 * (C) KetteQ, Inc.
 */

#include "util.h"

#include <ctype.h>
#include <stdio.h>
#include <malloc.h>

char * convertIntToStr(int number) {
    char * returnStr;
    int number_len = snprintf(NULL, 0, "%d", number);
    returnStr = malloc((number_len + 1) * sizeof (char));
    snprintf(returnStr, number_len+1, "%d", number);
    return returnStr;
}

char * convertUIntToStr(uint number) {
    char * returnStr;
    int number_len = snprintf(NULL, 0, "%d", number);
    returnStr = malloc((number_len + 1) * sizeof (char));
    snprintf(returnStr, number_len+1, "%d", number);
    return returnStr;
}

/**
 * From here: https://www.geeksforgeeks.org/binary-search/
 * @param arr
 * @param left
 * @param right
 * @param value
 * @return
 */
int coutil_binary_search(int arr[], int left, int right, int value) {
    if (right == left)
        return right;
    if (right > left) {
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
    // Return -1 if not found.
    return -1;
}

/**
 * This a modified binary search algo. that returns the upper limit index.
 * @param arr
 * @param left
 * @param right
 * @param value
 * @return
 */
int coutil_left_binary_search(const int arr[], int left, int right, int value) {
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (arr[mid] < value)
            left = mid + 1;
        else if (arr[mid] > value) {
            right = mid - 1;
        } else
            return mid;
    }
    return left - 1;
}

/**
 * Replaces the chars of the given pointer with their corresponding lowercase char.
 * @param data
 */
void coutil_str_to_lowercase(char * data) {
    for (; *data; ++data) * data = tolower(*data);
}