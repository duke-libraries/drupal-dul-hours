<?php
?>
<div class="hours-row<?php echo $item->isCurrentDay ? ' hours-today' : ''; ?>">
	<span class="day"><?php echo $item->libraryDate['dayAndDate']; ?></span>
	<span class="hours"><?php echo $item->hours_encoded; ?></span>
	<?php if(!empty($public_item)): ?>
	<div class="genpublic">Public: <?php echo $public_item->hours; ?></div>
	<?php endif; ?>
</div>
