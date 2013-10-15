<?php

/**
 * Display the hours for all library locations in one
 * table grid
 */
function dul_hours_table_page($form, &$form_state) {
	
	// TODO add 'weight' column to duke_cal_library table
	$weight = array(
		'lsc' => 100,
		'lilly' => -2,
		'perkins' => -10,
		'rubenstein' => 0,
		'public' => -1,
		'music' => 1,
		'uarchives' => -9,
	);
	
	require_once drupal_get_path('module', 'dul_hours') . '/dul_hours.calendar.inc';
	
	// ROADMAP:
	// 1) Retrieve the library ids (perkins, public, etc)
	$libraries = dul_hours_load_libraries();
	$service = dul_hours_load_calendar_service();

	// Determine closest "previous" Sunday and then the "upcoming" Saturday
	$now = time();

	if (isset($form_state['starttime'])) {
		$now = $form_state['starttime'];
	}
	
	$delta_day = (int) date('w', $now);
	$sunday_time = $now - (86400 * $delta_day);
	$saturday_time = $now + ((86400 * (7 - $delta_day)) - 1); // seven days in a week, yes?
	$page_subtitle = strftime('%A, %B %e', $sunday_time) . ', ' . strftime('%A, %B %e', $saturday_time);
	
	$header = array('');
	for ($i = 0; $i < 7; $i++) {
		$header[] = strftime('%A<br />%b %e', $sunday_time + (86400 * $i));
	}
	
	$rows = array();
	
	// 2) For each, fetch a week's worth of data
	//    -- determine closest "previous" Sunday and upcoming "Saturday"
	foreach($libraries as $library) {
		$day_start = time();
	
		$calevents = array();
		$events = dul_hours_fetch_events($library->google_cal_id, $service, $sunday_time, 7);
		$row['data'] = array($library->library_name . '<div>' . l(t('View by month'), 'about/hours/' . $library->library_id));
		foreach($events->items as $eventItem) {
			$hours_record = dul_hours_create_hours_record($library, $eventItem);
			$calevents[] = $hours_record;
			$row['data'][] = array('data' => $hours_record->hours, 'width' => '12%');
		}
		$rows[] = $row;

		$library->calevents = $calevents;
		$library->weight = $weight[$library->library_id];
	}
	
	$form['navigation'] = array(
		'#type' => 'container',
		'#attributes' => array(
			'class' => array('hours-table-buttons'),
		)
	);
	
	#get the 'next' previous sunday
	$previous_sunday = $sunday_time - (7 * 86400);
	$next_sunday = $sunday_time + (7 * 86400);
	$form['time_previous'] = array(
		'#type' => 'hidden',
		'#value' => $previous_sunday,
	);
	$form['time_next'] = array(
		'#type' => 'hidden',
		'#value' => $next_sunday,
	);
	$form['navigation']['prev'] = array(
		'#type' => 'submit',
		'#value' => t('<< Prev'),
		'#suffix' => '&nbsp;',
	);
	$form['navigation']['next'] = array(
		'#type' => 'submit',
		'#value' => t('Next >>'),
	);
	
	$form['hoursTable'] = array(
		'#theme' => 'table',
		'#header' => $header,
		'#rows' => $rows,
		'#attributes' => array(
			'class' => array('table'),
		),
	);
	$form['#submit'][] = 'dul_hours_table_page_submit';
	return $form;
}

function dul_hours_table_page_submit($form, &$form_state) {
	if ($form_state['values']['op'] == 'Next >>') {
		$form_state['starttime'] = $form_state['values']['time_next'];
	} else if ($form_state['values']['op'] == '<< Prev') {
		$form_state['starttime'] = $form_state['values']['time_previous'];
	}
	$form_state['rebuild'] = TRUE;
}

/**
 * Display the hours for a single library in a calendar table view
 * TODO: Investigate leveraging the Views module for this.
 */
function dul_hours_calendar_page($form, &$form_state, $library) {
	require_once drupal_get_path('module', 'dul_hours') . '/dul_hours.calendar.inc';
	$service = dul_hours_load_calendar_service();

	#1) find the number of days in the month
	#2) get the current week number
	#3) generate an array of weeks
	#   foreach week, fetch the events from google
	#
	$now = time();
	if (isset($form_state['starttime'])) {
		$now = $form_state['starttime'];
	}
	$firstday_of_month = mktime(0, 0, 0, date('n', $now), 1, date('Y', $now));
	$week['first'] = (int) date('W', $firstday_of_month);
	$num_days_in_month = date('t');
	$lastday_of_month = mktime(23, 59, 59, date('n', $now), $num_days_in_month, date('Y', $now));
	$week['last'] = (int) date('W', $lastday_of_month);
	
	$delta_day = (int) date('w', $firstday_of_month);
	$sunday_time = $firstday_of_month - (86400 * $delta_day);
	
	$delta_day = (int) date('w', $lastday_of_month);
	$saturday_time = $lastday_of_month + ((86400 * (7 - $delta_day)) - 1); // seven days in a week, yes?
	
	$weeks = array();
	//dpm($week['first']);
	//dpm($week['last']);
	for ($i = $week['first']; $i <= $week['last']; $i++) {
		// since, according to PHP's date() function, the first of a week is a Monday,
		// it's possible for the last week to have a starting date beyond the target month
		//if (date('M', $sunday_time) != date('M', $firstday_of_month)) {
		//	break;	// we're done processing. 
		//}


		$events = dul_hours_fetch_events($library->google_cal_id, $service, $sunday_time, 7);

		$calevents = array();
		foreach($events->items as $eventItem) {
			$hours_record = dul_hours_create_hours_record($library, $eventItem, date('M', $firstday_of_month));
			$calevents[] = $hours_record;
			// $row['data'][] = array('data' => $hours_record->hours, 'width' => '12%');
		}
		//kpr($calevents);
		$weeks[] = $calevents;
		$sunday_time += (86400 * 7);
	}

	$table = theme('library_calendar', array('library' => $library, 'weeks' => $weeks));
	//$render = render($table);

	$render['library_calendar'] = array(
		'#theme' => 'library_calendar',
		'#library' => $library,
		'#weeks' => $weeks,
	);

	return $render;
}

function dul_hours_calendar_page_submit($form, &$form_state) {
	
}

function theme_library_calendar($variables) {
	$header = array('Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat');
	$rows = array();
	$weeks = $variables['weeks'];

	foreach ($weeks as $week) {
		$row = array('data' => array());
		foreach ($week as $event) {
			$row['data'][] = $event->libraryDate['dateStr'] . '<br />' . $event->hours;
		}
		$rows[] = $row;
	}
	return render(theme('table', array('header' => $header, 'rows' => $rows)));
}
