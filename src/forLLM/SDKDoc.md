1.6获取对象 ID  
PQKit 中每一个对象都有一个唯一的 ID 标识与其对应,PQKit 与调用者之间的数据交互都需要通过 ID 来维系。  
PQKit 中有多种方式可以获取到对象的 ID。静态获取有两个 api 来获取  对象 ID，
它们都返回对象名称和 ID 的匹配对,需要调用者根据对象名称来匹  配正确 ID。动态获取参考章节 1.29。 
pq_GetAllDataObjectsByType 返回的 o_sNames 是指定类型的多个对  象名称中间添加“#”拼接的字符串,
如“C 型架地轨#变位机#ER8_1500#”,  o_sIDs 是指定类型的多个对象 ID 中间添加“#”拼接的字符串,
如“53687  0913#536870914#536870915#”。相应的 C 型架地轨的 ID 为 536870913,  
变位机的 ID 为 536870914,以此类推 ER8_1500 的 ID 为 536870915。

HRESULT pq_GetAllDataObjectsByType (PQDataType i_eObjType,BSTR* o_sName  s,BSTR* o_sIDs)
i_eObjType 指定对象类型,枚举类型值详见 9.1 数据类型定义
o_sNames 所有指定类型对象的名字,以“#”分割  
o_sIDs 所有指定类型对象的 ID,以“#”分割

#c++参考代码段
CComBSTR bsName = _T("");  CComBSTR bsID = _T("");  
m_ptrKit->pq_GetAllDataObjectsByType(PQ_ROBOT, &bsName, &bsID);




#数据类型定义：
ObjType ID  机器人 PQ_ROBOT  工件 PQ_WORKINGPART  工具 PQ_TOOL  底座 PQ_SEAT  轨迹 PQ_PATH  轨迹点 PQ_POINT  场景文件 PQ_ACCESSORY_PART  工作单元 PQ_WORKCELL  状态机 PQ_MECHSTATE  坐标系 PQ_COORD  轨迹文件夹 PQ_PATH_FOLDER