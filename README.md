# adbe
mysql的存储引擎插件。ADBE = A database engine

受Database Design And Implementation的启发。遂将书配套的SimpleDB3.3 Java写的数据库，用C++重新实现。并将其作为Mysql 5.7.22的一个存储引擎插件，取名为ADBE。到目前为止，已经实现了FileMgr,LogMgr,BufferMgr。其它模块正逐步实现中。进一步与mysql整合还未开始做。