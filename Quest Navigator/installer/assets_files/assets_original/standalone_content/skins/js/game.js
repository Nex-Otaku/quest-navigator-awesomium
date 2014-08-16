/* Функции скина для игры */

function qspSkinOnInitApi() {
	// Для мобильников в портретном режиме отключаем обычные скроллы.
	// Потестить на девайсах, может и не пригодится.
	if ($(window).width() <= 480) {
	/*
		if (qsp_iScroll_main != null) {
			qsp_iScroll_main.destroy();
			qsp_iScroll_main = null;
		}
		if (qsp_iScroll_acts != null) {
			qsp_iScroll_acts.destroy();
			qsp_iScroll_acts = null;
		}
		if (qsp_iScroll_vars != null) {
			qsp_iScroll_vars.destroy();
			qsp_iScroll_vars = null;
		}
		if (qsp_iScroll_objs != null) {
			qsp_iScroll_objs.destroy();
			qsp_iScroll_objs = null;
		}
		*/
	}
}


/* Собственные функции скина */ 
function skinToggleInv() {
	$("#qsp-inv").slideToggle();
	$("#skin-inv-toggle").toggleClass('open');
}

// Гасим сплэш-скрин.
function skinHideSplashScreen() {
	$("#skin-ui-wrapper").show();
	$("#skin-splashscreen-foreground").fadeOut('slow');
}
// Показываем сплэш-скрин.
function skinShowSplashScreen(callback) {
	$("#skin-splashscreen-foreground").fadeIn('slow', callback);
}

/* Функции скина для игры */

// Колбэки

function qspSkinOnDeviceSet() {
	// Вызывается, когда мы узнали, на каком устройстве запущена игра
	var more_games_link = 'http://qsp.su';
	if (qspIsAndroid) {
		more_games_link = 'market://search?q=pub:Butterfly+Lantern';
	} else if (qspIsIos) {
		more_games_link = 'itms-apps://itunes.apple.com/ru/artist/butterfly-lantern-interactive/id508671395';
	}
	$("#more-games-button a").attr('href', more_games_link);
}

function qspSkinOnSetGroupedContent() {
	// При первом вызове список пуст.
	// После первого заполнения список уже не пуст, 
	// поэтому вызов выполняться не будет.
	if (qspLocalGames === null) {
		// Запрашиваем список локальных игр.
		// Когда ответ будет получен, список заполнится автоматически.
		QspLib.listLocalGames();
	}
}

function qspSkinOnFillLocalGamesList() {
	// Гасим сплэш-скрин по завершению заполнения списка игр.
	skinHideSplashScreen();
}

function qspSkinOnSelectLocalGameInGamestock(hash) {
	// Показываем сплэш-скрин при выборе игры из списка.
	// Тем самым обеспечиваем плавный переход.
	skinShowSplashScreen(function() {
		// По завершению анимации вызываем библиотечный метод для запуска игры из списка.
		QspLib.selectLocalGameInGamestock(hash);
	});
	// Так как мы запускаем игру из скина, отменяем запуск в API.
	// Без этого у нас не получилось бы сделать анимацию перехода.
	return false;
}

function qspSkinOnSave() {
	//skinHideInv();
}

function qspSkinOnLoad() {
	//skinHideInv();
}

function qspSkinOnRestart() {
	//skinHideInv();
}

// Свои функции

var skinHideScrollsOriginal = 0;
var skinInv = false;
var skinMusic = true;
var skinStage = "";

function skinToggleMusic() {
	skinMusic = !skinMusic;
	skinSetMusicButton();
	QspLib.setMute(!skinMusic);
}

function skinHideScrolls(immediate) {
	qspGameSkin.hideScrollMain = ((skinHideScrollsOriginal === 1) || skinInv) ? 1 : 0;
	if (immediate) {
		qspApplyScrollsVisibility();
	}
}

function skinSetMusicButton() {
	skinToggleButton('#qsp-user-music img', '(button_music_)(on|off)(_pressed)?', '$1' + (skinMusic ? 'on' : 'off') + '$3');
}

function skinSetStage(cssClass) {
	// Переключаем класс всего body, тем самым задаем разный стиль для разных игровых экранов
	var t = $(document.body);
	if ((skinStage !== '') && (t.hasClass(skinStage))) {
		t.removeClass(skinStage);
	}
	skinStage = cssClass;
	if ((cssClass !== '') && (!t.hasClass(cssClass))) {
		t.addClass(cssClass);
	}
}

function skinToggleButton(selector, pattern, replacement) {
	var t = $(selector);
	if (t.length == 0)
		return;
	var re = new RegExp(pattern, "g");
	var btn1 = t.attr('src').replace(re, replacement);
	var btn2 = t.attr('data-pressed').replace(re, replacement);
	t.attr('src', btn1);
	t.attr('data-pressed', btn2);
}
