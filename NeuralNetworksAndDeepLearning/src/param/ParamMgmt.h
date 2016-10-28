/**
 * @file ParamMgmt.h
 * @date 2016-10-27
 * @author mhlee
 * @brief 파라미터를 관리하는 클래스
 * @details
 *  (1) hot area에서 사용이 되는 파라미터는 그 값의 읽기 성능이 중요하다.
 *      그래서 그 값을 O(1)의 시간으로 접근할 수 있도록 Param 클래스에서 구현 되었다.
 *      (Param.h에 정의되어 있는 SPARAM() 매크로를 이용하여 접근한다.)
 *  (2) 파라미터 값을 쓰는 행위는 작업을 동작시키기 전에 1회 수행이 된다. 따라서 쓰기 성능은
 *  중요하지 않다.
 *  (3) 기타 파라미터 관리 행위 역시 빈번하게 일어나지 않는다.
 *  (4) cold area에 해당이 되는 (2),(3)의 작업은 ParamMgmt 클래스에서 수행이 된다.
 *      paramName을 키로하여 해시테이블 형태로 구현이 되어 있는 STL의 Map을 사용하였다.
 *  (5) 참고로 ParamMgmt의 getValue()함수는 관리용으로만 활용을 하고, hot area에서는 
 *      사용하지 않는다.
 */

#ifndef PARAMMGMT_H
#define PARAMMGMT_H 

#include <map>
#include <string>

#include "ParamDef.h"

using namespace std;

class ParamMgmt {
public:
    enum ParamType : int {
        UINT8 = 0,
        INT8,
        UINT16,
        INT16,
        UINT32,
        INT32,
        UINT64,
        INT64,
        BOOL,
        FLOAT,
        DOUBLE,
        LONGDOUBLE,
        STRING,
        MAX
    };
                                    ParamMgmt() {}
    virtual                        ~ParamMgmt() {}
    static void                     initParamDefMap();
    static bool                     isParamExist(string paramName);

    static void                     getValue(string paramName, void* value);
    static void                     setValue(string paramName, void* value);
    static char*                    getDesc(string paramName);
    static bool                     isMandatory(string paramName);
    static bool                     isMutable(string paramName);
    static bool                     isSessScope(string paramName);
    static char*                    getTypeName(string paramName);
    static ParamType                getParamType(string paramName);
    static char*                    getDefaultValue(string paramName);

private:
    static map<string, ParamDef*>   paramDefMap;
};

#endif /* PARAMMGMT_H */
