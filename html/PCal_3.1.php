<!DOCTYPE html>
<html>
        <head>
                <title>Clipped</title>
                <meta charset='utf-8'>
                <meta name='viewport' content='width=device-width, initial-scale=1'>
                <link rel='stylesheet' href='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.css' />
                <script src='http://code.jquery.com/jquery-1.9.1.min.js'></script>
                <script src='http://code.jquery.com/mobile/1.3.2/jquery.mobile-1.3.2.min.js'></script>
                <style>
                        .ui-header .ui-title { margin-left: 1em; margin-right: 1em; text-overflow: clip; }
                </style>
        </head>
		<body>
<div data-role="page" id="page1">
    <div data-theme="a" data-role="header" data-position="fixed">
        <div class="ui-grid-a">
            <div class="ui-block-a">
                <input id="cancel" type="submit" data-theme="c" data-icon="delete" data-iconpos="left"
                value="Cancel" data-mini="true">
            </div>
            <div class="ui-block-b">
                <input id="save" type="submit" data-theme="b" data-icon="check" data-iconpos="right"
                value="Save" data-mini="true">
            </div>
        </div>
    </div>
    <div data-role="content">

<div data-role="fieldcontain">
	<label for="lang">
		Language
	</label>
	<select id="lang" data-native-menu="true" name="lang" data-mini="true">

<?php
	$langs = array(
				   0 => 'Dutch',
				   1 => 'English',
				   2 => 'French',
				   3 => 'German',
				   5 => 'Italian',
				   4 => 'Spanish',
				   6 => 'Swedish'
				   );
	
	if (!isset($_GET['lang'])) {
		$lang = 1;
	} else {
		$lang = $_GET['lang'];
	}
	
	foreach ($langs as $v => $n) {
		if ($lang == $v) {
			$s = " selected";
		} else {
			$s = "";
		}
		echo '<option value="' . $v . '" ' . $s . '>' . $n . '</option>';
	}
	?>
</select>
</div>

<div id="weekstart" data-role="fieldcontain">
<fieldset data-role="controlgroup" data-type="horizontal" data-mini="true">
<legend>Week starts on</legend>

<?php
	if (!isset($_GET['weekstart'])) {
		$weekstart = 0; // Default to Sunday
	} else {
		$weekstart = $_GET['weekstart'];
	}
	
	if ($weekstart == 0) {
		$s1 = " checked";
		$s2 = "";
	} else {
		$s1 = "";
		$s2 = " checked";
	}
	
	echo '<input id="format1" name="weekstart" value="0" data-theme="" type="radio"' . $s1 . '><label for="format1">Sunday</label>';
	echo '<input id="format2" name="weekstart" value="1" data-theme="" type="radio"' . $s2 . '><label for="format2">Monday</label>';
	?>
</fieldset>
</div>


<div data-role="fieldcontain">
            <label for="showweeknum">
                Show Week Number
			</label>
            <select name="showweeknum" id="showweeknum" data-theme="" data-role="slider" data-mini="true">
<?php
	if (!isset($_GET['showweeknum'])) {
		$showweeknum = 1;
	} else {
		$showweeknum = $_GET['showweeknum'];
	}
	
	if ($showweeknum == 0) {
		$s1 = " selected";
		$s2 = "";
	} else {
		$s1 = "";
		$s2 = " selected";
	}
	echo '<option value="0"' . $s1 .'>Off</option><option value="1"' . $s2 . '>On</option>';
?>
            </select>
        </div>

<div data-role="fieldcontain">
	<label for="nwd_country">
		Non Working Days Country
	</label>
	<select id="nwd_country" data-native-menu="true" name="nwd_country" data-mini="true">

<?php
	$countries = array(
			0 => 'None',
			1 => 'U.S.A.',
			2 => 'France',
			3 => 'Sweden'
	);
	
	if (!isset($_GET['nwd_country'])) {
		$nwd_country = 0;
	} else {
		$nwd_country = $_GET['nwd_country'];
	}
	
	foreach ($countries as $v => $n) {
		if ($nwd_country == $v) {
			$s = " selected";
		} else {
			$s = "";
		}
		echo '<option value="' . $v . '" ' . $s . '>' . $n . '</option>';
	}
	?>
</select>
</div>

</div>

    <script>
      function saveOptions() {
        var options = {
			'lang': parseInt($("#lang").val(), 10),
			'weekstart': parseInt($("input[name=weekstart]:checked").val(), 10),
			'showweeknum': parseInt($("#showweeknum").val(), 10),
			'nwd_country': parseInt($("#nwd_country").val(), 10),
        }
        return options;
      }

      $().ready(function() {
        $("#cancel").click(function() {
          console.log("Cancel");
          document.location = "pebblejs://close#";
        });

        $("#save").click(function() {
          console.log("Submit");
          
          var location = "pebblejs://close#" + encodeURIComponent(JSON.stringify(saveOptions()));
          console.log("Close: " + location);
          console.log(location);
          document.location = location;
        });

      });
    </script>
</body>
</html>
