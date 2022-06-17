<?php
// Connect to database
$server = "localhost";
$user = "ttt@localhost"; 
$pass = "1";
$dbname = "weatherStation";

$conn = mysqli_connect($server,$user,$pass,$dbname);

// Check connection
if($conn === false){
    die("ERROR: Could not connect. " . mysqli_connect_error());
}

?>