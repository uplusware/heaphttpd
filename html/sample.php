<?php
    if($_SERVER['REQUEST_METHOD'] == 'POST')
        echo "PHP Post Sample ".$_POST["abc"]." + ".$_POST["def"]." = ".($_POST["abc"] + $_POST["def"]);
    else if($_SERVER['REQUEST_METHOD'] == 'GET')
        echo "PHP GET Sample ".$_GET["abc"]." + ".$_GET["def"]." = ".($_GET["abc"] + $_GET["def"]);
    else
        echo "Wrong HTTP METHOD: ".$_SERVER['REQUEST_METHOD'];
?>
