all: new_plots sync

collect_data:
	python3 readfrommqtt.py localhost

clean:
	rm -rf plots

new_plots:
	cp rgbsensortest_data.sqlite3 /tmp
	python3 plotdata.py /tmp/rgbsensortest_data.sqlite3

sync:
	git pull
	git add plots/*.png
	git commit -m "Today's light"
	git push
