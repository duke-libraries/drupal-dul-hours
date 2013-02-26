<?php

/**
 * @file
 *  Drush command to update the hours data for the 
 *  library calendar used by a particular site.
 *
 *  Caveat: If 'perkins' is the site, we'll also need to update
 *  the 'public' hours data, irregardless of whether a site admin
 *  has set the 'display public hours' flag.
 */

/**
 * Implementation of hook_drush_command().
 */
function dul_hours_drush_command() {
	$items = array();

	// the 'dul-refresh-hours' command
	$items['dul-refresh-hours'] = array(
		'description' => 'Load the Google calendar data for a given library (i.e \'perkins\', etc)',
		'arguments' => array(
			'library' => 'Library for which to fetch calendar events.  Defaults to the site\'s designated library.',
		),
		'options' => array(
			'num-events' => 'How many events to fetch from Google.  Defaults to 30',
		),
		'examples' => array(
			'drush dhr perkins' => 'Fetch Google Calendar events from Perkins/Bostock library.',
			'drush dhr --num-events=7' => 'Fetch 7 Google Calendar events from site\'s default library.',
		),
		'aliases' => array('dhr','hours'),
		'bootstrap' => DRUSH_BOOTSTRAP_FULL,
	);

	return $items;
}

function drush_dul_hours_dul_refresh_hours_validate() {
	return TRUE;
}

function drush_dul_hours_dul_refresh_hours($library = NULL) {
	$default_library = variable_get('dul_hours.default_library', 'perkins');
	drush_print("\n" . $default_library . "\n");
	watchdog('dul_hours', 'Completed hours refresh');

	// delete hours row in which the enddate is greater than 'now'
	$records_deletd = dul_hours_delete_hours(time(), TRUE);

	require_once DRUPAL_ROOT . '/sites/all/libraries/gapi/apiClient.php';
	require_once DRUPAL_ROOT . '/sites/all/libraries/gapi/contrib/apiCalendarService.php';

	$query = db_select('duke_cal_library', 'l')
	->fields('l')
	->condition('library_id', $default_library);
	
	$result = $query->execute();
	if (!count($result)) {
		drush_log(dt('No Google Calendar Feed ID was found for ' . $default_library), 'error');
		echo sprintf("No Google calendar feed ID for %s\n", $default_library);
		exit(1);
	}

	$library = $result->fetchObject();
	print_r($library);
	echo sprintf("google_cal_id = [%s]\n", $library->google_cal_id);
	drush_log(dt("Google Calendar Feed = [" . $library->google_cal_id . "]"), info);

	require_once drupal_get_path('module', 'dul_hours') . '/dul_hours.calendar.inc';
	$service = dul_hours_load_calendar_service();
	$day_start = time();
	$events = dul_hours_fetch_events($library->google_cal_id, $service, $day_start, $day_end);

	drush_print_r($events);
}
