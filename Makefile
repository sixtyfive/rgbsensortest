all:
	scp rennsemmel:/home/jrs/rgbsensortest_data.sqlite3 .
	python3 plotdata.py

clean:
	rm -rf rgbsensortest_data.sqlite3 plots
