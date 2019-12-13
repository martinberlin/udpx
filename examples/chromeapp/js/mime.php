<?php
$f = isset($_GET['f']) ? $_GET['f'] : null;
if (is_null($f)) {
    exit('f not defined');
}

header("Content-Type: application/wasm");
header("Access-Control-Allow-Origin: *");
readfile($f);
