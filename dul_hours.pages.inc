<?php

/**
 * Display the hours for all library locations in one
 * table grid
 */
function dul_hours_table_page($form, &$form_state) {

	# RT 261619
	# WebX request to add links to the professional school libraries
	# @see affiliate_hours.tpl.php for default theme implementation
	#
	# set the weight to 100 to force to the bottom.
	$form['affiliate_hours'] = array(
		'#theme' => 'affiliate_hours',
		'#weight' => 100
	);
		
	// don't bother with Google API on dev and pre VMs 
	// because of the proxy firewall.
	if (preg_match('/(pre|dev)/', $_SERVER['SERVER_NAME']) == 1) {
		// will not be able to fetch data from Google due to proxy complications
		// exit quietly with an empty string
		$form['no_load'] = array(
			'#type' => 'markup',
			'#markup' => '<p>' . t('Unable to fetch Google Calenar data in pre-production or development environment') . '</p>',
		);
		return $form;
	}
	
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
	
	# include the interface to the Google APIs
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
		
	// don't bother with Google API on dev and pre VMs 
	// because of the proxy firewall.
	if (preg_match('/(pre|dev)/', $_SERVER['SERVER_NAME']) == 0) {
		// will not be able to fetch data from Google due to proxy complications
		// exit quietly with an empty string

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

			/*
			if (sizeof($calevents) < 7) {
				for ($i = 6; $i > sizeof($calevents); $i--) {
					$emptyRec = new stdClass();
					$emptyRec->emptyRecord = TRUE;
					$calevents[] = $emptyRec;
				}
			}
			*/

			//kpr($calevents);
			$weeks[] = $calevents;
			$sunday_time += (86400 * 7);
		}
	}

	# set the 'time_next' to the first day of the next month.
	$time_next = mktime(0, 0, 0, intval(date('n', $lastday_of_month)) + 1, 1, date('Y', $lastday_of_month));
	
	# set the 'time_prev' to the first day of the previous month
	$time_prev = mktime(0, 0, 0, intval(date('n', $firstday_of_month)) - 1, 1, date('Y', $firstday_of_month));

	$form['date_string'] = array(
		'#type' => 'markup',
		'#markup' => '<h3>' . t(date('F Y', $now)) . '</h3>',
	);
	$form['time_previous'] = array(
		'#type' => 'hidden',
		'#value' => $time_prev,
	);
	$form['time_next'] = array(
		'#type' => 'hidden',
		'#value' => $time_next,
	);
	$form['navigation'] = array(
		'#type' => 'container',
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
	$form['library_calendar'] = array(
		'#theme' => 'library_calendar',
		'#library' => $library,
		
		'#weeks' => $weeks,
	);

	return $form;
}

function dul_hours_calendar_page_submit($form, &$form_state) {
	if ($form_state['values']['op'] == 'Next >>') {
		$form_state['starttime'] = $form_state['values']['time_next'];
	} else if ($form_state['values']['op'] == '<< Prev') {
		$form_state['starttime'] = $form_state['values']['time_previous'];
	}
	$form_state['rebuild'] = TRUE;
}

function theme_library_calendar($variables) {
	$header = array('Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat');
	$rows = array();
	$weeks = $variables['weeks'];

	foreach ($weeks as $week) {
		$row = array('data' => array());
		foreach ($week as $event) {
			if (!isset($week->emptyRecord)) {
				$row['data'][] = $event->libraryDate['dateStr'] . '<br />' . $event->hours;
			} else {
				$row['data'][] = '';
			}
		}
		$rows[] = $row;
	}
	return render(theme('table', array('header' => $header, 'rows' => $rows, 'empty' => t('Unable to fetch Google Calendar data.'))));
}
