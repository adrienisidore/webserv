<?php
// Simple PHP CGI Script
// Save this as 'simple_cgi.php' and make it executable: chmod +x simple_cgi.php

// Set the content type header
header('Content-Type: text/html; charset=UTF-8');

// Start HTML output
?>
<!DOCTYPE html>
<html>
<head>
    <title>Simple PHP CGI Script</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        h1 { color: #333; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        tr:nth-child(even) { background-color: #f9f9f9; }
    </style>
</head>
<body>
    <h1>Simple PHP CGI Script</h1>

    <h2>Request Information</h2>
    <table>
        <tr><th>Method</th><td><?= $_SERVER['REQUEST_METHOD'] ?? 'N/A' ?></td></tr>
        <tr><th>Query String</th><td><?= $_SERVER['QUERY_STRING'] ?? 'N/A' ?></td></tr>
        <tr><th>Script Name</th><td><?= $_SERVER['SCRIPT_NAME'] ?? 'N/A' ?></td></tr>
        <tr><th>User Agent</th><td><?= $_SERVER['HTTP_USER_AGENT'] ?? 'N/A' ?></td></tr>
    </table>

    <h2>Form Data (GET/POST)</h2>
    <?php if (!empty($_REQUEST)): ?>
    <table>
        <?php foreach ($_REQUEST as $key => $value): ?>
        <tr><th><?= htmlspecialchars($key) ?></th><td><?= htmlspecialchars($value) ?></td></tr>
        <?php endforeach; ?>
    </table>
    <?php else: ?>
    <p>No form data submitted.</p>
    <?php endif; ?>

    <h2>Environment Variables</h2>
    <table>
        <?php foreach ($_SERVER as $key => $value): ?>
        <tr><th><?= htmlspecialchars($key) ?></th><td><?= htmlspecialchars($value) ?></td></tr>
        <?php endforeach; ?>
    </table>

    <h2>Test Form</h2>
    <form method="post" action="<?= htmlspecialchars($_SERVER['SCRIPT_NAME']) ?>">
        <p>
            <label for="name">Your Name:</label>
            <input type="text" id="name" name="name">
        </p>
        <p>
            <label for="message">Message:</label>
            <textarea id="message" name="message" rows="4"></textarea>
        </p>
        <p>
            <input type="submit" value="Submit">
        </p>
    </form>
</body>
</html>

