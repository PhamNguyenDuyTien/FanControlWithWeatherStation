<?php
// doc du lieu tu website gui ve
$tempRange1 = $_POST["temp-range1"];
$tempRange2 = $_POST["temp-range2"];
$tempRange3 = $_POST["temp-range3"];

// ket noi database
include("config.php");

// gui data xuong database
$sql = "update auto set range1=$tempRange1, range2=$tempRange2, range3=$tempRange3 , isUpdated=1";
mysqli_query($conn, $sql);

$sql1 = "update control set isUpdated=1, isUpdatedAuto=1";
mysqli_query($conn, $sql1);

// ngat ket noi voi database
mysqli_close($conn);
?>