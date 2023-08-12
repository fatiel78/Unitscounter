<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta http-equiv="refresh" content="2">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Document</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            line-height: 1.6;
            background-color: #f9f9f9;
            margin: 0;
            padding: 0;
        }

        h2 {
            color: #333;
            text-align: center;
        }

        table {
            border-collapse: collapse;
            width: 100%;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            background-color: #fff;
            border-radius: 5px;
            overflow: hidden;
            margin: 20px auto;
        }

        th, td {
            border: 1px solid #ccc;
            padding: 12px;
            text-align: left;
        }

        th {
            background-color: #f2f2f2;
        }

        tr:nth-child(even) {
            background-color: #f2f2f2;
        }

        tr:hover {
            background-color: #e5e5e5;
        }

        /* Responsive styles */
        @media screen and (max-width: 600px) {
            table {
                font-size: 14px;
            }
            th, td {
                padding: 8px;
            }
        }
    </style>
</head>
<body>
    <h2>Time of the passage of units Table</h2>
    <?php
    $connection = pg_connect("host=localhost dbname=DARI user=postgres password=root");
    if (!$connection) {
        echo "An error has occurred.<br>";
        exit;
    }

    $result = pg_query($connection, "SELECT * FROM units");
    if (!$result) {
        echo "An error has occurred.<br>";
        exit;
    }

    // Fetch all rows and store them in an array
    $rows = array();
    while ($row = pg_fetch_assoc($result)) {
        $rows[] = $row;
    }

    // Reverse the order of the rows
    $reversedRows = array_reverse($rows);
    ?>

    <table>
        <tr>
            <th>IDCONV</th>
            <th>DT</th>
            <th>ID</th>
        </tr>
        <?php
        foreach ($reversedRows as $row) {
            echo "
            <tr>
                <td>{$row['idconv']}</td>
                <td>{$row['dateheure']}</td>          
                <td>{$row['ID']}</td>
            </tr>
            ";
        }
        ?>
    </table>
</body>
</html>
