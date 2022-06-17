<?php
// ket noi database
include("config.php");

switch($_GET['act']){
    case "ChangeMode":
        $mode = $_GET['mode'] == "true"?1:0;
        $sql = "update control set fanMode=$mode, isUpdated=1 where id=1";
        mysqli_query($conn, $sql);
        echo json_encode($mode);
        break;
    case "ChangeLevel":
        $level = $_GET['level'];
        $sql = "update control set fanLevel=$level, isUpdated=1 where id=1";
        mysqli_query($conn, $sql);
        echo json_encode($level);
        break;
        
}
// ngat ket noi voi database
mysqli_close($conn);
?>