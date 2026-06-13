//
// Created by Cherry on 2026/6/11.
//

#ifndef STORAGE_TYPES_H
#define STORAGE_TYPES_H

typedef enum {
    STORAGE_TYPE_REGISTRY = 1,   // device settings registry (instance 0)
    STORAGE_TYPE_KEYMAP   = 2,   /* 一键一对象；instance = 源扫描码 */

} storage_type_t;

#endif //STORAGE_TYPES_H

