all: clean cpdb plots

plots:
	python3 plotdata.py

cpdb:
	scp rennsemmel:/home/jrs/rgbsensortest_data.sqlite3 .

clean:
	rm -rf plots
